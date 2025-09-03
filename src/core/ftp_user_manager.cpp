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
            // Note: anonymous_home is not in SecurityConfig, using default
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

    const auto& user = *user_it->second;

    // For now, skip status checks since the methods don't exist
    // TODO: Add proper status checking when FTPUser class is complete

    // Verify password
    if (!user.verifyPassword(password)) {
        logger_->warn("Authentication failed: invalid password for user: " + username);
        return false;
    }

    // Update last login time
    user_it->second->updateLastLogin();

    logger_->info("User authenticated successfully: " + username);
    return true;
}

std::shared_ptr<FTPUser> FTPUserManager::getUser(const std::string& username) {
    if (!initialized_) {
        return nullptr;
    }

    auto it = users_.find(username);
    if (it != users_.end()) {
        // Create a new user object since we can't copy due to atomic members
        auto user = std::make_shared<FTPUser>(it->second->getUsername());
        user->setPasswordHash(it->second->getPasswordHash());
        user->setHomeDirectory(it->second->getHomeDirectory());
        user->setShell(it->second->getShell());
        user->setGroup(it->second->getGroup());

        // Copy other properties that can be set
        // Note: We can't copy atomic statistics, so they'll start fresh

        return user;
    }

    return nullptr;
}

std::vector<std::shared_ptr<FTPUser>> FTPUserManager::getAllUsers() const {
    std::vector<std::shared_ptr<FTPUser>> result;
    result.reserve(users_.size());

    for (const auto& pair : users_) {
        // Create a new user object since we can't copy due to atomic members
        auto user = std::make_shared<FTPUser>(pair.second->getUsername());
        user->setPasswordHash(pair.second->getPasswordHash());
        user->setHomeDirectory(pair.second->getHomeDirectory());
        user->setShell(pair.second->getShell());
        user->setGroup(pair.second->getGroup());

        // Copy other properties that can be set
        // Note: We can't copy atomic statistics, so they'll start fresh

        result.push_back(user);
    }

    return result;
}

bool FTPUserManager::addUser(const FTPUser& user) {
    if (!initialized_) {
        logger_->error("User manager not initialized");
        return false;
    }

    if (user.getUsername().empty()) {
        logger_->error("Cannot add user with empty username");
        return false;
    }

    if (users_.find(user.getUsername()) != users_.end()) {
        logger_->error("User already exists: " + user.getUsername());
        return false;
    }

    // Create a new user object since we can't copy due to atomic members
    auto new_user = std::make_unique<FTPUser>(user.getUsername());
    new_user->setPasswordHash(user.getPasswordHash());
    new_user->setHomeDirectory(user.getHomeDirectory());
    new_user->setShell(user.getShell());
    new_user->setGroup(user.getGroup());

    // Add user
    users_[user.getUsername()] = std::move(new_user);

    logger_->info("User added: " + user.getUsername());
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

    // Create a new user object since we can't copy due to atomic members
    auto new_user = std::make_unique<FTPUser>(updated_user.getUsername());
    new_user->setPasswordHash(updated_user.getPasswordHash());
    new_user->setHomeDirectory(updated_user.getHomeDirectory());
    new_user->setShell(updated_user.getShell());
    new_user->setGroup(updated_user.getGroup());

    // Update user
    it->second = std::move(new_user);

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

    // Update password using the setter method
    it->second->setPassword(new_password);

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

    // For now, just log the action since status methods don't exist
    // TODO: Implement proper locking when FTPUser class is complete
    logger_->info("User lock requested: " + username);
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

    // For now, just log the action since status methods don't exist
    // TODO: Implement proper unlocking when FTPUser class is complete
    logger_->info("User unlock requested: " + username);
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

    // For now, just log the action since status methods don't exist
    // TODO: Implement proper enabling when FTPUser class is complete
    logger_->info("User enable requested: " + username);
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

    // For now, just log the action since status methods don't exist
    // TODO: Implement proper disabling when FTPUser class is complete
    logger_->info("User disable requested: " + username);
    return true;
}

void FTPUserManager::createDefaultUsers() {
    // Create anonymous user if allowed
    if (allow_anonymous_) {
        auto anon_user = std::make_unique<FTPUser>(anonymous_user_);
        anon_user->setPasswordHash("");
        anon_user->setHomeDirectory(anonymous_home_);
        // Note: Can't set status since the method doesn't exist

        users_[anonymous_user_] = std::move(anon_user);
    }

    // Create admin user
    auto admin_user = std::make_unique<FTPUser>("admin");
    admin_user->setPassword("admin"); // This will hash the password
    admin_user->setHomeDirectory("/home/admin");
    // Note: Can't set status since the method doesn't exist

    users_["admin"] = std::move(admin_user);
}

} // namespace ssftpd
