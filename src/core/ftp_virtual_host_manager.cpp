#include "ssftpd/ftp_virtual_host_manager.hpp"
#include "ssftpd/ftp_server_config.hpp"
#include "ssftpd/ftp_virtual_host.hpp"
#include <iostream>
#include <filesystem>

namespace ssftpd {

FTPVirtualHostManager::FTPVirtualHostManager(std::shared_ptr<FTPServerConfig> config,
                                             std::shared_ptr<Logger> logger)
    : config_(config)
    , logger_(logger)
    , initialized_(false)
{
}

FTPVirtualHostManager::~FTPVirtualHostManager() {
    stop();
}

bool FTPVirtualHostManager::initialize() {
    if (initialized_) {
        return true;
    }

    try {
        if (!config_) {
            logger_->error("No configuration provided for virtual host manager");
            return false;
        }

        // Load virtual hosts from configuration
        loadVirtualHosts();

        // Validate all virtual hosts
        if (!validateVirtualHosts()) {
            logger_->error("Virtual host validation failed");
            return false;
        }

        initialized_ = true;
        logger_->info("Virtual host manager initialized with " +
                     std::to_string(virtual_hosts_.size()) + " virtual hosts");
        return true;

    } catch (const std::exception& e) {
        logger_->error("Failed to initialize virtual host manager: " + std::string(e.what()));
        return false;
    }
}

void FTPVirtualHostManager::stop() {
    if (!initialized_) {
        return;
    }

    // Clear virtual hosts
    virtual_hosts_.clear();
    initialized_ = false;
    logger_->info("Virtual host manager stopped");
}

std::shared_ptr<FTPVirtualHost> FTPVirtualHostManager::getVirtualHost(const std::string& hostname) const {
    if (!initialized_) {
        return nullptr;
    }

    // Find virtual host by hostname
    for (const auto& vhost : virtual_hosts_) {
        if (vhost->getHostname() == hostname) {
            return vhost;
        }
    }

    // Return default virtual host if no specific match found
    return getDefaultVirtualHost();
}

std::shared_ptr<FTPVirtualHost> FTPVirtualHostManager::getDefaultVirtualHost() const {
    if (!initialized_ || virtual_hosts_.empty()) {
        return nullptr;
    }

    // Return the first virtual host as default
    return virtual_hosts_.front();
}

bool FTPVirtualHostManager::addVirtualHost(std::shared_ptr<FTPVirtualHost> virtual_host) {
    if (!virtual_host) {
        logger_->error("Cannot add null virtual host");
        return false;
    }

    if (virtual_host->getHostname().empty()) {
        logger_->error("Cannot add virtual host with empty hostname");
        return false;
    }

    // Check if virtual host already exists
    for (const auto& existing : virtual_hosts_) {
        if (existing->getHostname() == virtual_host->getHostname()) {
            logger_->error("Virtual host already exists: " + virtual_host->getHostname());
            return false;
        }
    }

    // Add the virtual host
    virtual_hosts_.push_back(virtual_host);
    logger_->info("Virtual host added: " + virtual_host->getHostname());
    return true;
}

bool FTPVirtualHostManager::updateVirtualHost(const std::string& hostname,
                                             std::shared_ptr<FTPVirtualHost> updated_vhost) {
    if (!updated_vhost) {
        logger_->error("Cannot update with null virtual host");
        return false;
    }

    // Find and update existing virtual host
    for (auto& vhost : virtual_hosts_) {
        if (vhost->getHostname() == hostname) {
            // Create a new virtual host with updated settings
            auto new_vhost = std::make_shared<FTPVirtualHost>(updated_vhost->getHostname());
            new_vhost->setDocumentRoot(updated_vhost->getDocumentRoot());
            new_vhost->setWelcomeMessage(updated_vhost->getWelcomeMessage());
            new_vhost->setBannerMessage(updated_vhost->getBannerMessage());

            // Replace the old one
            vhost = new_vhost;
            logger_->info("Virtual host updated: " + hostname);
            return true;
        }
    }

    logger_->error("Virtual host not found for update: " + hostname);
    return false;
}

bool FTPVirtualHostManager::removeVirtualHost(const std::string& hostname) {
    auto it = virtual_hosts_.begin();
    while (it != virtual_hosts_.end()) {
        if ((*it)->getHostname() == hostname) {
            virtual_hosts_.erase(it);
            logger_->info("Virtual host removed: " + hostname);
            return true;
        }
        ++it;
    }

    logger_->error("Virtual host not found for removal: " + hostname);
    return false;
}

std::vector<std::string> FTPVirtualHostManager::getVirtualHostNames() const {
    std::vector<std::string> names;
    names.reserve(virtual_hosts_.size());

    for (const auto& vhost : virtual_hosts_) {
        names.push_back(vhost->getHostname());
    }

    return names;
}

bool FTPVirtualHostManager::validateVirtualHosts() {
    std::vector<std::string> invalid_hosts;

    for (const auto& vhost : virtual_hosts_) {
        // Check hostname
        if (vhost->getHostname().empty()) {
            invalid_hosts.push_back("(no hostname)");
            continue;
        }

        // Check document root
        if (vhost->getDocumentRoot().empty()) {
            invalid_hosts.push_back(vhost->getHostname() + " (no document root)");
            continue;
        }

        // Check if document root exists
        if (!std::filesystem::exists(vhost->getDocumentRoot())) {
            invalid_hosts.push_back(vhost->getHostname() + " (document root does not exist: " +
                                   vhost->getDocumentRoot() + ")");
            continue;
        }

        // Check if document root is a directory
        if (!std::filesystem::is_directory(vhost->getDocumentRoot())) {
            invalid_hosts.push_back(vhost->getHostname() + " (document root is not a directory: " +
                                   vhost->getDocumentRoot() + ")");
            continue;
        }
    }

    if (!invalid_hosts.empty()) {
        logger_->error("Virtual host validation failed:");
        for (const auto& invalid : invalid_hosts) {
            logger_->error("  - " + invalid);
        }
        return false;
    }

    return true;
}

void FTPVirtualHostManager::loadVirtualHosts() {
    if (!config_ || !config_->virtual_hosts.empty()) {
        return;
    }

    // Create virtual hosts from configuration
    for (const auto& vhost_config : config_->virtual_hosts) {
        if (vhost_config.enabled) {
            auto virtual_host = std::make_shared<FTPVirtualHost>(vhost_config.hostname);
            virtual_host->setDocumentRoot(vhost_config.document_root);
            virtual_host->setWelcomeMessage(vhost_config.welcome_message);
            virtual_host->setBannerMessage(vhost_config.banner_message);

            virtual_hosts_.push_back(virtual_host);
        }
    }

    logger_->info("Loaded " + std::to_string(virtual_hosts_.size()) + " virtual hosts from configuration");
}

size_t FTPVirtualHostManager::getVirtualHostCount() const {
    return virtual_hosts_.size();
}

bool FTPVirtualHostManager::hasVirtualHost(const std::string& hostname) const {
    for (const auto& vhost : virtual_hosts_) {
        if (vhost->getHostname() == hostname) {
            return true;
        }
    }
    return false;
}

} // namespace ssftpd
