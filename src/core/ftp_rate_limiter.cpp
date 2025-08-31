#include "ssftpd/ftp_rate_limiter.hpp"
#include "ssftpd/logger.hpp"
#include <chrono>
#include <algorithm>

namespace ssftpd {

FTPRateLimiter::FTPRateLimiter(std::shared_ptr<FTPServerConfig> config, 
                               std::shared_ptr<Logger> logger)
    : config_(config)
    , logger_(logger)
    , initialized_(false)
    , max_connections_per_ip_(10)
    , max_connections_per_minute_(100)
    , max_requests_per_minute_(1000)
    , connection_window_(std::chrono::minutes(1))
    , request_window_(std::chrono::minutes(1))
{
}

FTPRateLimiter::~FTPRateLimiter() = default;

bool FTPRateLimiter::initialize() {
    if (initialized_) {
        return true;
    }
    
    try {
        if (config_ && config_->rate_limit.enabled) {
            // Load rate limiting configuration
            max_connections_per_ip_ = config_->rate_limit.max_connections_per_minute;
            max_connections_per_minute_ = config_->rate_limit.max_connections_per_minute;
            max_requests_per_minute_ = config_->rate_limit.max_requests_per_minute;
            
            // Convert time windows from seconds to chrono duration
            connection_window_ = std::chrono::seconds(config_->rate_limit.window_size);
            request_window_ = std::chrono::seconds(config_->rate_limit.window_size);
        }
        
        initialized_ = true;
        logger_->info("FTP rate limiter initialized");
        return true;
        
    } catch (const std::exception& e) {
        logger_->error("Failed to initialize rate limiter: " + std::string(e.what()));
        return false;
    }
}

bool FTPRateLimiter::allowConnection(const std::string& ip_address) {
    if (!initialized_) {
        return true;
    }
    
    auto now = std::chrono::steady_clock::now();
    
    // Clean up old connection records
    cleanupOldRecords(ip_connections_, connection_window_);
    
    // Check connection limit per IP
    auto& ip_conns = ip_connections_[ip_address];
    if (ip_conns.size() >= max_connections_per_ip_) {
        logger_->warn("Rate limit exceeded for IP " + ip_address + 
                     ": max connections per IP reached");
        return false;
    }
    
    // Check global connection limit
    size_t total_connections = 0;
    for (const auto& pair : ip_connections_) {
        total_connections += pair.second.size();
    }
    
    if (total_connections >= max_connections_per_minute_) {
        logger_->warn("Global connection rate limit exceeded");
        return false;
    }
    
    // Record new connection
    ip_conns.push_back(now);
    
    return true;
}

bool FTPRateLimiter::allowRequest(const std::string& ip_address) {
    if (!initialized_) {
        return true;
    }
    
    auto now = std::chrono::steady_clock::now();
    
    // Clean up old request records
    cleanupOldRecords(ip_requests_, request_window_);
    
    // Check request limit per IP
    auto& ip_reqs = ip_requests_[ip_address];
    if (ip_reqs.size() >= max_requests_per_minute_) {
        logger_->warn("Rate limit exceeded for IP " + ip_address + 
                     ": max requests per minute reached");
        return false;
    }
    
    // Record new request
    ip_reqs.push_back(now);
    
    return true;
}

void FTPRateLimiter::cleanupOldRecords(std::map<std::string, std::vector<std::chrono::steady_clock::time_point>>& records,
                                       const std::chrono::steady_clock::duration& window) {
    auto now = std::chrono::steady_clock::now();
    auto cutoff = now - window;
    
    for (auto& pair : records) {
        auto& timestamps = pair.second;
        
        // Remove timestamps older than the window
        timestamps.erase(
            std::remove_if(timestamps.begin(), timestamps.end(),
                [cutoff](const auto& timestamp) {
                    return timestamp < cutoff;
                }),
            timestamps.end()
        );
    }
    
    // Remove empty IP entries
    auto it = records.begin();
    while (it != records.end()) {
        if (it->second.empty()) {
            it = records.erase(it);
        } else {
            ++it;
        }
    }
}

void FTPRateLimiter::setMaxConnectionsPerIP(size_t max_connections) {
    max_connections_per_ip_ = max_connections;
    logger_->info("Max connections per IP set to: " + std::to_string(max_connections));
}

void FTPRateLimiter::setMaxConnectionsPerMinute(size_t max_connections) {
    max_connections_per_minute_ = max_connections;
    logger_->info("Max connections per minute set to: " + std::to_string(max_connections));
}

void FTPRateLimiter::setMaxRequestsPerMinute(size_t max_requests) {
    max_requests_per_minute_ = max_requests;
    logger_->info("Max requests per minute set to: " + std::to_string(max_requests));
}

void FTPRateLimiter::setConnectionWindow(std::chrono::seconds window) {
    connection_window_ = window;
    logger_->info("Connection window set to: " + std::to_string(window.count()) + " seconds");
}

void FTPRateLimiter::setRequestWindow(std::chrono::seconds window) {
    request_window_ = window;
    logger_->info("Request window set to: " + std::to_string(window.count()) + " seconds");
}

std::map<std::string, size_t> FTPRateLimiter::getConnectionStats() const {
    std::map<std::string, size_t> stats;
    
    for (const auto& pair : ip_connections_) {
        stats[pair.first] = pair.second.size();
    }
    
    return stats;
}

std::map<std::string, size_t> FTPRateLimiter::getRequestStats() const {
    std::map<std::string, size_t> stats;
    
    for (const auto& pair : ip_requests_) {
        stats[pair.first] = pair.second.size();
    }
    
    return stats;
}

void FTPRateLimiter::reset() {
    ip_connections_.clear();
    ip_requests_.clear();
    logger_->info("Rate limiter statistics reset");
}

} // namespace ssftpd
