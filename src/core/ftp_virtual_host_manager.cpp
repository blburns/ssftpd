#include "ssftpd/ftp_virtual_host_manager.hpp"
#include "ssftpd/logger.hpp"
#include <filesystem>

namespace ssftpd {

FTPVirtualHostManager::FTPVirtualHostManager(std::shared_ptr<FTPServerConfig> config, 
                                            std::shared_ptr<Logger> logger)
    : config_(config)
    , logger_(logger)
    , initialized_(false)
{
}

FTPVirtualHostManager::~FTPVirtualHostManager() = default;

bool FTPVirtualHostManager::initialize() {
    if (initialized_) {
        return true;
    }
    
    try {
        if (config_ && config_->enable_virtual_hosts) {
            // Load virtual hosts from configuration
            virtual_hosts_ = config_->virtual_hosts;
            
            // Validate virtual hosts
            validateVirtualHosts();
        }
        
        initialized_ = true;
        logger_->info("FTP virtual host manager initialized with " + 
                     std::to_string(virtual_hosts_.size()) + " virtual hosts");
        
        return true;
        
    } catch (const std::exception& e) {
        logger_->error("Failed to initialize virtual host manager: " + std::string(e.what()));
        return false;
    }
}

std::shared_ptr<FTPVirtualHost> FTPVirtualHostManager::getVirtualHost(const std::string& hostname) {
    if (!initialized_) {
        return nullptr;
    }
    
    for (const auto& vhost : virtual_hosts_) {
        if (vhost.hostname == hostname) {
            return std::make_shared<FTPVirtualHost>(vhost);
        }
    }
    
    return nullptr;
}

std::vector<std::shared_ptr<FTPVirtualHost>> FTPVirtualHostManager::getAllVirtualHosts() const {
    std::vector<std::shared_ptr<FTPVirtualHost>> result;
    result.reserve(virtual_hosts_.size());
    
    for (const auto& vhost : virtual_hosts_) {
        result.push_back(std::make_shared<FTPVirtualHost>(vhost));
    }
    
    return result;
}

bool FTPVirtualHostManager::addVirtualHost(const FTPVirtualHost& vhost) {
    if (!initialized_) {
        logger_->error("Virtual host manager not initialized");
        return false;
    }
    
    if (vhost.hostname.empty()) {
        logger_->error("Cannot add virtual host with empty hostname");
        return false;
    }
    
    // Check if hostname already exists
    for (const auto& existing : virtual_hosts_) {
        if (existing.hostname == vhost.hostname) {
            logger_->error("Virtual host already exists: " + vhost.hostname);
            return false;
        }
    }
    
    // Add virtual host
    virtual_hosts_.push_back(vhost);
    
    logger_->info("Virtual host added: " + vhost.hostname);
    return true;
}

bool FTPVirtualHostManager::updateVirtualHost(const std::string& hostname, const FTPVirtualHost& updated_vhost) {
    if (!initialized_) {
        logger_->error("Virtual host manager not initialized");
        return false;
    }
    
    for (auto& vhost : virtual_hosts_) {
        if (vhost.hostname == hostname) {
            vhost = updated_vhost;
            logger_->info("Virtual host updated: " + hostname);
            return true;
        }
    }
    
    logger_->error("Virtual host not found: " + hostname);
    return false;
}

bool FTPVirtualHostManager::deleteVirtualHost(const std::string& hostname) {
    if (!initialized_) {
        logger_->error("Virtual host manager not initialized");
        return false;
    }
    
    auto it = virtual_hosts_.begin();
    while (it != virtual_hosts_.end()) {
        if (it->hostname == hostname) {
            virtual_hosts_.erase(it);
            logger_->info("Virtual host deleted: " + hostname);
            return true;
        }
        ++it;
    }
    
    logger_->error("Virtual host not found: " + hostname);
    return false;
}

void FTPVirtualHostManager::validateVirtualHosts() {
    std::vector<std::string> invalid_hosts;
    
    for (const auto& vhost : virtual_hosts_) {
        if (vhost.hostname.empty()) {
            invalid_hosts.push_back("empty hostname");
            continue;
        }
        
        if (vhost.document_root.empty()) {
            invalid_hosts.push_back(vhost.hostname + " (no document root)");
            continue;
        }
        
        if (!std::filesystem::exists(vhost.document_root)) {
            invalid_hosts.push_back(vhost.hostname + " (document root does not exist: " + vhost.document_root + ")");
        }
    }
    
    if (!invalid_hosts.empty()) {
        logger_->warn("Found " + std::to_string(invalid_hosts.size()) + " invalid virtual hosts:");
        for (const auto& invalid : invalid_hosts) {
            logger_->warn("  " + invalid);
        }
    }
}

} // namespace ssftpd
