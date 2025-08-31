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

void FTPServerConfig::setDefaults() {
    // Reset all configuration to defaults
    ssl = SSLConfig{};
    logging = LoggingConfig{};
    security = SecurityConfig{};
    transfer = TransferConfig{};
    connection = ConnectionConfig{};
    passive = PassiveConfig{};
    rate_limit = RateLimitConfig{};
    
    virtual_hosts.clear();
    users.clear();
    
    server_name = "Simple FTP Daemon";
    server_version = "0.1.0";
    server_banner = "Welcome to Simple FTP Daemon";
    config_file.clear();
    pid_file.clear();
    daemon_mode = false;
    foreground_mode = false;
    working_directory.clear();
    user_config_dir.clear();
    system_config_dir.clear();
    
    enable_ssl = false;
    enable_virtual_hosts = false;
    enable_user_management = true;
    enable_rate_limiting = false;
    enable_logging = true;
    enable_statistics = true;
    enable_monitoring = false;
    
    thread_pool_size = 4;
    max_memory_usage = 100 * 1024 * 1024; // 100MB
    enable_compression = false;
    enable_caching = true;
    cache_size = 10 * 1024 * 1024; // 10MB
    
    enable_metrics = false;
    metrics_endpoint = "/metrics";
    metrics_port = 8080;
    metrics_interval = std::chrono::seconds(60);
    
    enable_backup = false;
    backup_directory.clear();
    backup_interval = std::chrono::seconds(86400); // 24 hours
    max_backups = 7;
    
    debug_mode = false;
    verbose_logging = false;
    trace_commands = false;
    profile_performance = false;
    log_socket_events = "none";
    
    // Clear error and warning lists
    errors_.clear();
    warnings_.clear();
    loaded_ = false;
    last_modified_.clear();
    config_format_.clear();
}

bool FTPServerConfig::loadFromFile(const std::string& config_file) {
    this->config_file = config_file;
    
    if (!std::filesystem::exists(config_file)) {
        errors_.push_back("Configuration file does not exist: " + config_file);
        return false;
    }
    
    try {
        // Try to determine file format
        std::string extension = std::filesystem::path(config_file).extension().string();
        
        if (extension == ".json") {
            config_format_ = "json";
            return parseJSONConfig(config_file);
        } else if (extension == ".ini" || extension == ".conf") {
            config_format_ = "ini";
            return parseINIConfig(config_file);
        } else {
            // Try to auto-detect format
            return parseConfigFile(config_file);
        }
    } catch (const std::exception& e) {
        errors_.push_back("Error loading configuration file: " + std::string(e.what()));
        return false;
    }
    
    return false;
}

bool FTPServerConfig::loadFromJSON(const std::string& json_config) {
    config_format_ = "json";
    
    try {
        // For now, we'll just set a basic configuration
        // In a full implementation, you would parse the JSON string
        warnings_.push_back("JSON parsing not fully implemented - using defaults");
        
        // Set some basic values from the JSON if possible
        // This is a placeholder for actual JSON parsing
        
        loaded_ = true;
        return true;
    } catch (const std::exception& e) {
        errors_.push_back("Error parsing JSON configuration: " + std::string(e.what()));
        return false;
    }
}

bool FTPServerConfig::saveToFile(const std::string& config_file) const {
    try {
        std::ofstream file(config_file);
        if (!file.is_open()) {
            return false;
        }
        
        if (config_format_ == "json") {
            file << saveToJSON();
        } else {
            // Save as INI format
            file << "# Simple FTP Daemon Configuration File" << std::endl;
            file << "# Generated automatically" << std::endl << std::endl;
            
            file << "[server]" << std::endl;
            file << "server_name = " << server_name << std::endl;
            file << "server_version = " << server_version << std::endl;
            file << "enable_ssl = " << (enable_ssl ? "true" : "false") << std::endl;
            file << "enable_virtual_hosts = " << (enable_virtual_hosts ? "true" : "false") << std::endl;
            
            file << std::endl << "[connection]" << std::endl;
            file << "bind_address = " << connection.bind_address << std::endl;
            file << "bind_port = " << connection.bind_port << std::endl;
            file << "max_connections = " << connection.max_connections << std::endl;
            
            file << std::endl << "[logging]" << std::endl;
            file << "log_level = " << logging.log_level << std::endl;
            file << "log_file = " << logging.log_file << std::endl;
        }
        
        file.close();
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

std::string FTPServerConfig::saveToJSON() const {
    std::ostringstream oss;
    
    oss << "{" << std::endl;
    oss << "  \"server\": {" << std::endl;
    oss << "    \"name\": \"" << server_name << "\"," << std::endl;
    oss << "    \"version\": \"" << server_version << "\"," << std::endl;
    oss << "    \"enable_ssl\": " << (enable_ssl ? "true" : "false") << "," << std::endl;
    oss << "    \"enable_virtual_hosts\": " << (enable_virtual_hosts ? "true" : "false") << std::endl;
    oss << "  }," << std::endl;
    
    oss << "  \"connection\": {" << std::endl;
    oss << "    \"bind_address\": \"" << connection.bind_address << "\"," << std::endl;
    oss << "    \"bind_port\": " << connection.bind_port << "," << std::endl;
    oss << "    \"max_connections\": " << connection.max_connections << std::endl;
    oss << "  }," << std::endl;
    
    oss << "  \"logging\": {" << std::endl;
    oss << "    \"log_level\": \"" << logging.log_level << "\"," << std::endl;
    oss << "    \"log_file\": \"" << logging.log_file << "\"" << std::endl;
    oss << "  }" << std::endl;
    oss << "}" << std::endl;
    
    return oss.str();
}

bool FTPServerConfig::validate() const {
    errors_.clear();
    warnings_.clear();
    
    // Validate SSL configuration
    if (enable_ssl && !validateSSL()) {
        // SSL validation errors are already added to errors_ list
    }
    
    // Validate security configuration
    if (!validateSecurity()) {
        // Security validation errors are already added to errors_ list
    }
    
    // Validate connection configuration
    if (!validateConnection()) {
        // Connection validation errors are already added to errors_ list
    }
    
    // Validate virtual host configuration
    if (enable_virtual_hosts && !validateVirtualHosts()) {
        // Virtual host validation errors are already added to errors_ list
    }
    
    // Validate user configuration
    if (enable_user_management && !validateUsers()) {
        // User validation errors are already added to errors_ list
    }
    
    // Check for warnings
    if (connection.bind_address == "0.0.0.0") {
        warnings_.push_back("Binding to 0.0.0.0 allows connections from any IP address");
    }
    
    if (security.allow_anonymous) {
        warnings_.push_back("Anonymous access is enabled - consider security implications");
    }
    
    if (connection.max_connections > 1000) {
        warnings_.push_back("High connection limit may impact performance");
    }
    
    return errors_.empty();
}

std::vector<std::string> FTPServerConfig::getErrors() const {
    return errors_;
}

std::vector<std::string> FTPServerConfig::getWarnings() const {
    return warnings_;
}

void FTPServerConfig::merge(const FTPServerConfig& other) {
    // Merge configuration from another config
    // This is a simplified implementation
    
    if (!other.server_name.empty()) server_name = other.server_name;
    if (!other.server_version.empty()) server_version = other.server_version;
    if (!other.server_banner.empty()) server_banner = other.server_banner;
    
    // Merge other settings as needed
    // This is a placeholder for full merge functionality
}

void FTPServerConfig::reset() {
    setDefaults();
}

bool FTPServerConfig::parseConfigFile(const std::string& config_file) {
    // Auto-detect format and parse accordingly
    std::ifstream file(config_file);
    if (!file.is_open()) {
        errors_.push_back("Cannot open configuration file: " + config_file);
        return false;
    }
    
    std::string first_line;
    std::getline(file, first_line);
    file.close();
    
    // Try to detect format
    if (first_line.find('{') != std::string::npos) {
        config_format_ = "json";
        return parseJSONConfig(config_file);
    } else if (first_line.find('[') != std::string::npos || first_line.find('=') != std::string::npos) {
        config_format_ = "ini";
        return parseINIConfig(config_file);
    } else {
        errors_.push_back("Cannot determine configuration file format");
        return false;
    }
}

bool FTPServerConfig::parseJSONConfig(const std::string& config_file) {
    // Placeholder for JSON parsing
    // In a full implementation, you would use a JSON library like jsoncpp
    warnings_.push_back("JSON parsing not fully implemented - using defaults");
    
    // For now, just mark as loaded with defaults
    loaded_ = true;
    return true;
}

bool FTPServerConfig::parseINIConfig(const std::string& config_file) {
    // Placeholder for INI parsing
    // In a full implementation, you would parse the INI file
    
    std::ifstream file(config_file);
    if (!file.is_open()) {
        errors_.push_back("Cannot open INI configuration file: " + config_file);
        return false;
    }
    
    std::string line;
    std::string current_section;
    
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // Check for section headers
        if (line[0] == '[' && line[line.length() - 1] == ']') {
            current_section = line.substr(1, line.length() - 2);
            continue;
        }
        
        // Parse key-value pairs
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // Apply configuration based on section and key
            applyConfigValue(current_section, key, value);
        }
    }
    
    file.close();
    loaded_ = true;
    return true;
}

void FTPServerConfig::applyConfigValue(const std::string& section, const std::string& key, const std::string& value) {
    if (section == "server") {
        if (key == "server_name") server_name = value;
        else if (key == "server_version") server_version = value;
        else if (key == "enable_ssl") enable_ssl = (value == "true");
        else if (key == "enable_virtual_hosts") enable_virtual_hosts = (value == "true");
    }
    else if (section == "connection") {
        if (key == "bind_address") connection.bind_address = value;
        else if (key == "bind_port") connection.bind_port = std::stoi(value);
        else if (key == "max_connections") connection.max_connections = std::stoul(value);
    }
    else if (section == "logging") {
        if (key == "log_level") logging.log_level = value;
        else if (key == "log_file") logging.log_file = value;
    }
    // Add more sections as needed
}

bool FTPServerConfig::validateSSL() const {
    if (!enable_ssl) return true;
    
    if (ssl.certificate_file.empty()) {
        errors_.push_back("SSL enabled but no certificate file specified");
        return false;
    }
    
    if (ssl.private_key_file.empty()) {
        errors_.push_back("SSL enabled but no private key file specified");
        return false;
    }
    
    if (!std::filesystem::exists(ssl.certificate_file)) {
        errors_.push_back("SSL certificate file does not exist: " + ssl.certificate_file);
        return false;
    }
    
    if (!std::filesystem::exists(ssl.private_key_file)) {
        errors_.push_back("SSL private key file does not exist: " + ssl.private_key_file);
        return false;
    }
    
    return true;
}

bool FTPServerConfig::validateSecurity() const {
    if (security.chroot_enabled && security.chroot_directory.empty()) {
        errors_.push_back("Chroot enabled but no directory specified");
        return false;
    }
    
    if (security.chroot_enabled && !std::filesystem::exists(security.chroot_directory)) {
        errors_.push_back("Chroot directory does not exist: " + security.chroot_directory);
        return false;
    }
    
    return true;
}

bool FTPServerConfig::validateConnection() const {
    if (connection.bind_port == 0) {
        errors_.push_back("Invalid bind port: 0");
        return false;
    }
    
    if (connection.bind_port > 65535) {
        errors_.push_back("Invalid bind port: " + std::to_string(connection.bind_port));
        return false;
    }
    
    if (connection.max_connections == 0) {
        errors_.push_back("Invalid max connections: 0");
        return false;
    }
    
    return true;
}

bool FTPServerConfig::validateVirtualHosts() const {
    for (const auto& vhost : virtual_hosts) {
        if (vhost.hostname.empty()) {
            errors_.push_back("Virtual host with empty hostname");
            return false;
        }
        
        if (vhost.document_root.empty()) {
            errors_.push_back("Virtual host " + vhost.hostname + " has no document root");
            return false;
        }
        
        if (!std::filesystem::exists(vhost.document_root)) {
            errors_.push_back("Virtual host " + vhost.hostname + " document root does not exist: " + vhost.document_root);
            return false;
        }
    }
    
    return true;
}

bool FTPServerConfig::validateUsers() const {
    for (const auto& user : users) {
        if (user.username.empty()) {
            errors_.push_back("User with empty username");
            return false;
        }
        
        if (user.home_directory.empty()) {
            errors_.push_back("User " + user.username + " has no home directory");
            return false;
        }
        
        if (!std::filesystem::exists(user.home_directory)) {
            errors_.push_back("User " + user.username + " home directory does not exist: " + user.home_directory);
            return false;
        }
    }
    
    return true;
}

} // namespace ssftpd
