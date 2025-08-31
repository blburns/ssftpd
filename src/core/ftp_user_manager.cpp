#include "ssftpd/ftp_user_manager.hpp"
#include "ssftpd/logger.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace ssftpd {

FTPUserManager::FTPUserManager(std::shared_ptr<FTPServerConfig> config, 
                              std::shared_ptr<Logger> logger)
    : config_(config)
    , logger_(logger)
    , initialized_(false)
    , allow_anonymous_(false)
    , anonymous_user_("anonymous")
    , anonymous_password_("")
    , anonymous_home_("/var/ftp")
{
}

FTPUserManager::~FTPUserManager() = default;

bool FTPUserManager::initialize() {
    if (initialized_) {
        return true;
    }
    
    try {
        // Load configuration
        if (config_) {
            allow_anonymous_ = config_->security.allow_anonymous;
            anonymous_home_ = config_->security.anonymous_home;
        }
        
        // Create default users
        createDefaultUsers();
        
        initialized_ = true;
        logger_->info("FTP user manager initialized with " + 
                     std::to_string(users_.size()) + " users");
        
        return true;
        
    } catch (const std::exception& e) {
        logger_->error("Failed to initialize user manager: " + std::string(e.what()));
        return false;
    }
}

bool FTPUserManager::authenticateUser(const std::string& username, const std::string& password) {
    if (!initialized_) {
        logger_->error("User manager not initialized");
        return false;
    }
    
    // Check for anonymous access
    if (username == anonymous_user_ && allow_anonymous_) {
        if (password.empty() || password == anonymous_password_) {
            logger_->info("Anonymous user authenticated");
            return true;
        }
    }
    
    // Find user
    auto user_it = users_.find(username);
    if (user_it == users_.end()) {
        logger_->warn("Authentication failed: user not found: " + username);
        return false;
    }
    
    const auto& user = user_it->second;
    
    // Check if user is enabled
    if (!user.enabled) {
        logger_->warn("Authentication failed: user disabled: " + username);
        return false;
    }
    
    // Check if user is locked
    if (user.locked) {
        logger_->warn("Authentication failed: user locked: " + username);
        return false;
    }
    
    // Simple password check (in production, use proper hashing)
    if (password != user.password_hash) {
        logger_->warn("Authentication failed: invalid password for user: " + username);
        return false;
    }
    
    // Update last login time
    user_it->second.last_login = std::chrono::system_clock::now();
    user_it->second.login_count++;
    
    logger_->info("User authenticated successfully: " + username);
    return true;
}

std::shared_ptr<FTPUser> FTPUserManager::getUser(const std::string& username) {
    if (!initialized_) {
        return nullptr;
    }
    
    auto it = users_.find(username);
    if (it != users_.end()) {
        return std::make_shared<FTPUser>(it->second);
    }
    
    return nullptr;
}

std::vector<std::shared_ptr<FTPUser>> FTPUserManager::getAllUsers() const {
    std::vector<std::shared_ptr<FTPUser>> result;
    result.reserve(users_.size());
    
    for (const auto& pair : users_) {
        result.push_back(std::make_shared<FTPUser>(pair.second));
    }
    
    return result;
}

bool FTPUserManager::addUser(const FTPUser& user) {
    if (!initialized_) {
        logger_->error("User manager not initialized");
        return false;
    }
    
    if (user.username.empty()) {
        logger_->error("Cannot add user with empty username");
        return false;
    }
    
    if (users_.find(user.username) != users_.end()) {
        logger_->error("User already exists: " + user.username);
        return false;
    }
    
    // Add user
    users_[user.username] = user;
    
    logger_->info("User added: " + user.username);
    return true;
}

bool FTPUserManager::updateUser(const std::string& username, const FTPUser& updated_user) {
    if (!initialized_) {
        logger_->error("User manager not initialized");
        return false;
    }
    
    auto it = users_.find(username);
    if (it == users_.end()) {
        logger_->error("User not found: " + username);
        return false;
    }
    
    // Update user
    it->second = updated_user;
    
    logger_->info("User updated: " + username);
    return true;
}

bool FTPUserManager::deleteUser(const std::string& username) {
    if (!initialized_) {
        logger_->error("User manager not initialized");
        return false;
    }
    
    if (username == anonymous_user_) {
        logger_->error("Cannot delete anonymous user");
        return false;
    }
    
    auto it = users_.find(username);
    if (it == users_.end()) {
        logger_->error("User not found: " + username);
        return false;
    }
    
    // Remove user
    users_.erase(it);
    
    logger_->info("User deleted: " + username);
    return true;
}

bool FTPUserManager::changePassword(const std::string& username, const std::string& new_password) {
    if (!initialized_) {
        logger_->error("User manager not initialized");
        return false;
    }
    
    auto it = users_.find(username);
    if (it == users_.end()) {
        logger_->error("User not found: " + username);
        return false;
    }
    
    // Update password
    it->second.password_hash = new_password;
    it->second.password_changed = std::chrono::system_clock::now();
    
    logger_->info("Password changed for user: " + username);
    return true;
}

bool FTPUserManager::lockUser(const std::string& username) {
    if (!initialized_) {
        logger_->error("User manager not initialized");
        return false;
    }
    
    auto it = users_.find(username);
    if (it == users_.end()) {
        logger_->error("User not found: " + username);
        return false;
    }
    
    it->second.locked = true;
    it->second.locked_at = std::chrono::system_clock::now();
    
    logger_->info("User locked: " + username);
    return true;
}

bool FTPUserManager::unlockUser(const std::string& username) {
    if (!initialized_) {
        logger_->error("User manager not initialized");
        return false;
    }
    
    auto it = users_.find(username);
    if (it == users_.end()) {
        logger_->error("User not found: " + username);
        return false;
    }
    
    it->second.locked = false;
    it->second.locked_at = std::chrono::time_point<std::chrono::system_clock>();
    
    logger_->info("User unlocked: " + username);
    return true;
}

bool FTPUserManager::enableUser(const std::string& username) {
    if (!initialized_) {
        logger_->error("User manager not initialized");
        return false;
    }
    
    auto it = users_.find(username);
    if (it == users_.end()) {
        logger_->error("User not found: " + username);
        return false;
    }
    
    it->second.enabled = true;
    
    logger_->info("User enabled: " + username);
    return true;
}

bool FTPUserManager::disableUser(const std::string& username) {
    if (!initialized_) {
        logger_->error("User manager not initialized");
        return false;
    }
    
    auto it = users_.find(username);
    if (it == users_.end()) {
        logger_->error("User not found: " + username);
        return false;
    }
    
    it->second.enabled = false;
    
    logger_->info("User disabled: " + username);
    return true;
}

void FTPUserManager::createDefaultUsers() {
    // Create anonymous user if allowed
    if (allow_anonymous_) {
        FTPUser anon_user;
        anon_user.username = anonymous_user_;
        anon_user.password_hash = "";
        anon_user.home_directory = anonymous_home_;
        anon_user.enabled = true;
        anon_user.locked = false;
        anon_user.system_user = false;
        anon_user.read_enabled = true;
        anon_user.write_enabled = false;
        anon_user.delete_enabled = false;
        anon_user.create_enabled = false;
        anon_user.rename_enabled = false;
        
        users_[anonymous_user_] = anon_user;
    }
    
    // Create admin user
    FTPUser admin_user;
    admin_user.username = "admin";
    admin_user.password_hash = "admin";
    admin_user.home_directory = "/home/admin";
    admin_user.enabled = true;
    admin_user.locked = false;
    admin_user.system_user = false;
    admin_user.read_enabled = true;
    admin_user.write_enabled = true;
    admin_user.delete_enabled = true;
    admin_user.create_enabled = true;
    admin_user.rename_enabled = true;
    
    users_["admin"] = admin_user;
}

} // namespace ssftpd
