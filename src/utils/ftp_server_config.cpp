#include "ssftpd/ftp_server_config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace ssftpd {

FTPServerConfig::FTPServerConfig() {
    setDefaults();
}

FTPServerConfig::~FTPServerConfig() = default;

bool FTPServerConfig::loadFromFile(const std::string& config_file) {
    if (config_file.empty()) {
        return false;
    }
    
    if (!std::filesystem::exists(config_file)) {
        return false;
    }
    
    // Determine file format
    std::string extension = std::filesystem::path(config_file).extension().string();
    
    if (extension == ".json") {
        return parseJSONConfig(config_file);
    } else if (extension == ".ini" || extension == ".conf") {
        return parseINIConfig(config_file);
    } else {
        // Try to auto-detect format
        return parseConfigFile(config_file);
    }
}

bool FTPServerConfig::loadFromJSON(const std::string& json_config) {
    // For now, just return true as a placeholder
    // TODO: Implement actual JSON parsing
    return true;
}

bool FTPServerConfig::validate() const {
    // Clear previous errors and warnings (using const_cast since we need to modify)
    const_cast<FTPServerConfig*>(this)->errors_.clear();
    const_cast<FTPServerConfig*>(this)->warnings_.clear();
    
    // Validate SSL configuration
    if (!validateSSL()) {
        // SSL validation errors are already added to errors_ vector
    }
    
    // Validate security configuration
    if (!validateSecurity()) {
        // Security validation errors are already added to errors_ vector
    }
    
    // Validate connection configuration
    if (!validateConnection()) {
        // Connection validation errors are already added to errors_ vector
    }
    
    // Validate virtual host configuration
    if (!validateVirtualHosts()) {
        // Virtual host validation errors are already added to errors_ vector
    }
    
    // Validate user configuration
    if (!validateUsers()) {
        // User validation errors are already added to errors_ vector
    }
    
    // Add warnings for potential issues
    const_cast<FTPServerConfig*>(this)->warnings_.push_back("Binding to 0.0.0.0 allows connections from any IP address");
    
    if (security.allow_anonymous) {
        const_cast<FTPServerConfig*>(this)->warnings_.push_back("Anonymous access is enabled - consider security implications");
    }
    
    if (connection.max_connections > 1000) {
        const_cast<FTPServerConfig*>(this)->warnings_.push_back("High connection limit may impact performance");
    }
    
    return errors_.empty();
}

std::vector<std::string> FTPServerConfig::getErrors() const {
    return errors_;
}

std::vector<std::string> FTPServerConfig::getWarnings() const {
    return warnings_;
}

bool FTPServerConfig::isLoaded() const {
    return loaded_;
}

std::string FTPServerConfig::getLastModified() const {
    return last_modified_;
}

std::string FTPServerConfig::getConfigFormat() const {
    return config_format_;
}

bool FTPServerConfig::parseConfigFile(const std::string& config_file) {
    // For now, just return true as a placeholder
    // TODO: Implement actual config file parsing
    loaded_ = true;
    last_modified_ = "unknown";
    config_format_ = "auto";
    return true;
}

bool FTPServerConfig::parseJSONConfig(const std::string& config_file) {
    // For now, just return true as a placeholder
    // TODO: Implement actual JSON parsing
    loaded_ = true;
    last_modified_ = "unknown";
    config_format_ = "json";
    return true;
}

bool FTPServerConfig::parseINIConfig(const std::string& config_file) {
    // For now, just return true as a placeholder
    // TODO: Implement actual INI parsing
    loaded_ = true;
    last_modified_ = "unknown";
    config_format_ = "ini";
    return true;
}

void FTPServerConfig::setDefaults() {
    // SSL defaults
    ssl.enabled = false;
    ssl.certificate_file = "";
    ssl.private_key_file = "";
    ssl.ca_certificate_file = "";
    ssl.cipher_suite = "TLS_AES_256_GCM_SHA384";
    ssl.require_client_cert = false;
    ssl.verify_peer = false;
    ssl.min_tls_version = 0x0301; // TLS 1.0
    ssl.max_tls_version = 0x0304; // TLS 1.3
    
    // Logging defaults
    logging.log_file = "/var/log/ssftpd/ssftpd.log";
    logging.log_level = "INFO";
    logging.log_to_console = true;
    logging.log_to_file = true;
    logging.log_commands = true;
    logging.log_transfers = true;
    logging.log_errors = true;
    logging.log_format = "default";
    logging.max_log_size = 10 * 1024 * 1024; // 10MB
    logging.max_log_files = 5;
    
    // Security defaults
    security.chroot_enabled = false;
    security.chroot_directory = "";
    security.drop_privileges = true;
    security.run_as_user = "ssftpd";
    security.run_as_group = "ssftpd";
    security.allow_anonymous = false;
    security.allow_guest = false;
    security.require_ssl = false;
    security.max_login_attempts = 3;
    security.login_timeout = std::chrono::seconds(30);
    security.session_timeout = std::chrono::seconds(3600);
    
    // Transfer defaults
    transfer.max_file_size = 0; // 0 = unlimited
    transfer.max_transfer_rate = 0; // 0 = unlimited
    transfer.allow_overwrite = true;
    transfer.allow_resume = true;
    transfer.temp_directory = "/tmp";
    transfer.buffer_size = 8192;
    transfer.use_sendfile = true;
    transfer.use_mmap = false;
    
    // Connection defaults
    connection.bind_address = "0.0.0.0";
    connection.bind_port = 21;
    connection.max_connections = 100;
    connection.max_connections_per_ip = 10;
    connection.connection_timeout = std::chrono::seconds(300);
    connection.data_timeout = std::chrono::seconds(300);
    connection.idle_timeout = std::chrono::seconds(600);
    connection.keep_alive = true;
    connection.keep_alive_interval = 60;
    connection.keep_alive_probes = 3;
    connection.tcp_nodelay = true;
    connection.reuse_address = true;
    connection.backlog = 50;
    
    // Passive mode defaults
    passive.enabled = true;
    passive.min_port = 1024;
    passive.max_port = 65535;
    passive.use_external_ip = false;
    passive.external_ip = "";
    
    // Rate limiting defaults
    rate_limit.enabled = false;
    rate_limit.max_connections_per_minute = 60;
    rate_limit.max_requests_per_minute = 1000;
    rate_limit.max_transfer_rate = 1024 * 1024; // 1MB/s
    rate_limit.window_size = std::chrono::seconds(60);
    rate_limit.block_duration = std::chrono::seconds(300);
    
    // Virtual host defaults
    enable_virtual_hosts = false;
    enable_user_management = true;
    enable_rate_limiting = false;
    enable_logging = true;
    enable_statistics = true;
    enable_monitoring = false;
    
    // Performance defaults
    thread_pool_size = 4;
    max_memory_usage = 100 * 1024 * 1024; // 100MB
    enable_compression = false;
    enable_caching = true;
    cache_size = 10 * 1024 * 1024; // 10MB
    
    // Monitoring defaults
    enable_metrics = false;
    metrics_endpoint = "/metrics";
    metrics_port = 8080;
    metrics_interval = std::chrono::seconds(60);
    
    // Backup defaults
    enable_backup = false;
    backup_directory = "";
    backup_interval = std::chrono::seconds(86400); // 24 hours
    max_backups = 7;
    
    // Development defaults
    debug_mode = false;
    verbose_logging = false;
    trace_commands = false;
    profile_performance = false;
    log_socket_events = "none";
}

bool FTPServerConfig::validateSSL() const {
    if (!ssl.enabled) {
        return true;
    }
    
    if (ssl.certificate_file.empty()) {
        const_cast<FTPServerConfig*>(this)->errors_.push_back("SSL enabled but no certificate file specified");
        return false;
    }
    
    if (ssl.private_key_file.empty()) {
        const_cast<FTPServerConfig*>(this)->errors_.push_back("SSL enabled but no private key file specified");
        return false;
    }
    
    if (!std::filesystem::exists(ssl.certificate_file)) {
        const_cast<FTPServerConfig*>(this)->errors_.push_back("SSL certificate file does not exist: " + ssl.certificate_file);
        return false;
    }
    
    if (!std::filesystem::exists(ssl.private_key_file)) {
        const_cast<FTPServerConfig*>(this)->errors_.push_back("SSL private key file does not exist: " + ssl.private_key_file);
        return false;
    }
    
    return true;
}

bool FTPServerConfig::validateSecurity() const {
    if (security.chroot_enabled && security.chroot_directory.empty()) {
        const_cast<FTPServerConfig*>(this)->errors_.push_back("Chroot enabled but no directory specified");
        return false;
    }
    
    if (security.chroot_enabled && !std::filesystem::exists(security.chroot_directory)) {
        const_cast<FTPServerConfig*>(this)->errors_.push_back("Chroot directory does not exist: " + security.chroot_directory);
        return false;
    }
    
    return true;
}

bool FTPServerConfig::validateConnection() const {
    if (connection.bind_port == 0) {
        const_cast<FTPServerConfig*>(this)->errors_.push_back("Invalid bind port: 0");
        return false;
    }
    
    if (connection.bind_port > 65535) {
        const_cast<FTPServerConfig*>(this)->errors_.push_back("Invalid bind port: " + std::to_string(connection.bind_port));
        return false;
    }
    
    if (connection.max_connections == 0) {
        const_cast<FTPServerConfig*>(this)->errors_.push_back("Invalid max connections: 0");
        return false;
    }
    
    return true;
}

bool FTPServerConfig::validateVirtualHosts() const {
    for (const auto& vhost : virtual_hosts) {
        if (vhost.hostname.empty()) {
            const_cast<FTPServerConfig*>(this)->errors_.push_back("Virtual host with empty hostname");
            continue;
        }
        
        if (vhost.document_root.empty()) {
            const_cast<FTPServerConfig*>(this)->errors_.push_back("Virtual host " + vhost.hostname + " has no document root");
            continue;
        }
        
        if (!std::filesystem::exists(vhost.document_root)) {
            const_cast<FTPServerConfig*>(this)->errors_.push_back("Virtual host " + vhost.hostname + " document root does not exist: " + vhost.document_root);
            continue;
        }
    }
    
    return true;
}

bool FTPServerConfig::validateUsers() const {
    // For now, just return true as a placeholder
    // TODO: Implement actual user validation
    return true;
}

} // namespace ssftpd
