#include "ssftpd/ftp_user.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <functional>

namespace ssftpd {

FTPUser::FTPUser(const std::string& username)
    : username_(username)
    , password_hash_("")
    , home_directory_("/home/" + username)
    , shell_("/bin/bash")
    , group_("users")
    , status_(UserStatus::ACTIVE)
    , anonymous_(false)
    , guest_(false)
    , max_connections_(1)
    , current_connections_(0)
    , max_transfer_rate_(0)
    , max_file_size_(0)
    , session_timeout_(3600)
    , last_login_time_("")
    , expiration_date_("")
    , auth_method_(AuthMethod::PASSWORD)
    , total_uploads_(0)
    , total_downloads_(0)
    , total_bytes_uploaded_(0)
    , total_bytes_downloaded_(0)
    , total_connections_(0)
    , failed_logins_(0)
{
    // Set default permissions
    permissions_.insert(UserPermission::READ);
    permissions_.insert(UserPermission::LIST);
    permissions_.insert(UserPermission::DOWNLOAD);
}

FTPUser::~FTPUser() = default;

void FTPUser::setPassword(const std::string& password) {
    password_hash_ = hashPassword(password);
}

bool FTPUser::verifyPassword(const std::string& password) const {
    if (password_hash_.empty()) {
        return false;
    }
    
    std::string hashed_input = hashPassword(password);
    return password_hash_ == hashed_input;
}

void FTPUser::updateLastLogin() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    last_login_time_ = oss.str();
    
    total_connections_++;
}

bool FTPUser::isExpired() const {
    if (expiration_date_.empty()) {
        return false;
    }
    
    // Simple expiration check - in production, implement proper date parsing
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d");
    std::string current_date = oss.str();
    
    return current_date > expiration_date_;
}

void FTPUser::setExpirationDate(const std::string& expiration_date) {
    expiration_date_ = expiration_date;
}

std::string FTPUser::hashPassword(const std::string& password) const {
    // Simple hash implementation - in production, use proper cryptographic hashing
    std::hash<std::string> hasher;
    return std::to_string(hasher(password));
}

bool FTPUser::isPathAllowed(const std::string& path) const {
    if (allowed_paths_.empty()) {
        return true; // No restrictions
    }
    
    for (const auto& allowed_path : allowed_paths_) {
        if (path.find(allowed_path) == 0) {
            return true;
        }
    }
    
    return false;
}

bool FTPUser::isPathDenied(const std::string& path) const {
    for (const auto& denied_path : denied_paths_) {
        if (path.find(denied_path) == 0) {
            return true;
        }
    }
    
    return false;
}

} // namespace ssftpd
