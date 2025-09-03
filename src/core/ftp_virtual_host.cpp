#include "ssftpd/ftp_virtual_host.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace ssftpd {

FTPVirtualHost::FTPVirtualHost(const std::string& hostname)
    : hostname_(hostname)
    , document_root_("/var/www/" + hostname)
    , welcome_message_("Welcome to " + hostname)
    , banner_message_("FTP Server Ready")
    , enabled_(true)
    , default_(false)
    , access_control_(VirtualHostAccess::ALLOW_ALL)
    , security_level_(VirtualHostSecurity::MEDIUM)
    , total_connections_(0)
    , total_transfers_(0)
    , total_bytes_transferred_(0)
    , creation_time_("")
    , last_modification_time_("")
{
    // Initialize default values
    initializeDefaults();
}

FTPVirtualHost::~FTPVirtualHost() = default;

void FTPVirtualHost::initializeDefaults() {
    // Set default SSL configuration
    ssl_config_.enabled = false;
    ssl_config_.certificate_file = "";
    ssl_config_.private_key_file = "";
    ssl_config_.ca_certificate_file = "";
    ssl_config_.verify_peer = false;
    ssl_config_.min_tls_version = 0x0303; // TLS 1.2

    // Set default security configuration
    security_config_.allow_anonymous = false;
    security_config_.require_ssl = false;
    security_config_.max_login_attempts = 3;
    security_config_.session_timeout = std::chrono::seconds(3600);

    // Set default transfer configuration
    transfer_config_.max_file_size = 0;
    transfer_config_.max_transfer_rate = 0;
    transfer_config_.allowed_extensions = {};
    transfer_config_.denied_extensions = {};
    transfer_config_.allow_resume = true;

    // Set current timestamp
    creation_time_ = getCurrentTimestamp();
    last_modification_time_ = creation_time_;
}

std::string FTPVirtualHost::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

} // namespace ssftpd
