#include "ssftpd/ftp_connection_manager.hpp"
#include "ssftpd/logger.hpp"
#include <algorithm>
#include <chrono>
#include <thread>

namespace ssftpd {

FTPConnectionManager::FTPConnectionManager(std::shared_ptr<FTPServerConfig> config,
                                         std::shared_ptr<Logger> logger)
    : config_(config)
    , logger_(logger)
    , running_(false)
    , max_connections_(config ? config->connection.max_connections : 100)
    , connection_timeout_(std::chrono::seconds(300)) // 5 minutes
    , cleanup_interval_(std::chrono::seconds(60))   // 1 minute
{
}

FTPConnectionManager::~FTPConnectionManager() {
    stop();
}

bool FTPConnectionManager::start() {
    if (running_) {
        return true;
    }

    running_ = true;

    // Start cleanup thread
    cleanup_thread_ = std::thread(&FTPConnectionManager::cleanupLoop, this);

    logger_->info("FTP connection manager started");
    return true;
}

void FTPConnectionManager::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    // Wait for cleanup thread to finish
    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }

    // Close all connections
    std::lock_guard<std::mutex> lock(connections_mutex_);
    for (auto& connection : connections_) {
        if (connection && connection->isConnected()) {
            connection->disconnect();
        }
    }
    connections_.clear();

    logger_->info("FTP connection manager stopped");
}

bool FTPConnectionManager::addConnection(std::shared_ptr<FTPConnection> connection) {
    if (!connection) {
        logger_->error("Cannot add null connection");
        return false;
    }

    std::lock_guard<std::mutex> lock(connections_mutex_);

    // Check connection limit
    if (connections_.size() >= max_connections_) {
        logger_->warn("Connection limit reached, cannot add new connection");
        return false;
    }

    // Add connection
    connections_.push_back(connection);

    // Set connection start time
    connection->setStartTime(std::chrono::steady_clock::now());

    logger_->debug("Connection added, total connections: " + std::to_string(connections_.size()));
    return true;
}

void FTPConnectionManager::removeConnection(std::shared_ptr<FTPConnection> connection) {
    if (!connection) {
        return;
    }

    std::lock_guard<std::mutex> lock(connections_mutex_);

    auto it = std::find(connections_.begin(), connections_.end(), connection);
    if (it != connections_.end()) {
        connections_.erase(it);
        logger_->debug("Connection removed, total connections: " + std::to_string(connections_.size()));
    }
}

void FTPConnectionManager::processConnections() {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    auto it = connections_.begin();
    while (it != connections_.end()) {
        auto connection = *it;

        if (!connection) {
            // Remove null connections
            it = connections_.erase(it);
            continue;
        }

        // Check if connection is still alive
        if (!connection->isConnected()) {
            logger_->debug("Connection disconnected, removing");
            it = connections_.erase(it);
            continue;
        }

        // Process the connection
        try {
            connection->process();
        } catch (const std::exception& e) {
            logger_->error("Error processing connection: " + std::string(e.what()));
            connection->disconnect();
            it = connections_.erase(it);
            continue;
        }

        // Check connection timeout
        if (isConnectionTimedOut(connection)) {
            logger_->warn("Connection timed out, disconnecting");
            connection->disconnect();
            it = connections_.erase(it);
            continue;
        }

        ++it;
    }
}

bool FTPConnectionManager::isConnectionTimedOut(std::shared_ptr<FTPConnection> connection) const {
    if (!connection) {
        return true;
    }

    auto start_time = connection->getStartTime();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - start_time;

    return elapsed > connection_timeout_;
}

size_t FTPConnectionManager::getConnectionCount() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    return connections_.size();
}

std::vector<std::shared_ptr<FTPConnection>> FTPConnectionManager::getConnections() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    return connections_;
}

void FTPConnectionManager::disconnectAll() {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    for (auto& connection : connections_) {
        if (connection && connection->isConnected()) {
            connection->disconnect();
        }
    }

    logger_->info("All connections disconnected");
}

void FTPConnectionManager::disconnectByIP(const std::string& ip_address) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    size_t disconnected_count = 0;
    auto it = connections_.begin();

    while (it != connections_.end()) {
        auto connection = *it;

        if (connection && connection->getClientIP() == ip_address) {
            connection->disconnect();
            it = connections_.erase(it);
            disconnected_count++;
        } else {
            ++it;
        }
    }

    if (disconnected_count > 0) {
        logger_->info("Disconnected " + std::to_string(disconnected_count) +
                     " connections from IP: " + ip_address);
    }
}

void FTPConnectionManager::disconnectByUser(const std::string& username) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    size_t disconnected_count = 0;
    auto it = connections_.begin();

    while (it != connections_.end()) {
        auto connection = *it;

        if (connection && connection->getUsername() == username) {
            connection->disconnect();
            it = connections_.erase(it);
            disconnected_count++;
        } else {
            ++it;
        }
    }

    if (disconnected_count > 0) {
        logger_->info("Disconnected " + std::to_string(disconnected_count) +
                     " connections for user: " + username);
    }
}

void FTPConnectionManager::setMaxConnections(size_t max_connections) {
    max_connections_ = max_connections;
    logger_->info("Maximum connections set to: " + std::to_string(max_connections_));
}

void FTPConnectionManager::setConnectionTimeout(std::chrono::seconds timeout) {
    connection_timeout_ = timeout;
    logger_->info("Connection timeout set to: " + std::to_string(timeout.count()) + " seconds");
}

void FTPConnectionManager::setCleanupInterval(std::chrono::seconds interval) {
    cleanup_interval_ = interval;
    logger_->info("Cleanup interval set to: " + std::to_string(interval.count()) + " seconds");
}

void FTPConnectionManager::cleanupLoop() {
    while (running_) {
        // Sleep for cleanup interval
        std::this_thread::sleep_for(cleanup_interval_);

        if (!running_) {
            break;
        }

        // Perform cleanup
        cleanupConnections();
    }
}

void FTPConnectionManager::cleanupConnections() {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    size_t initial_count = connections_.size();
    auto it = connections_.begin();

    while (it != connections_.end()) {
        auto connection = *it;

        if (!connection) {
            // Remove null connections
            it = connections_.erase(it);
            continue;
        }

        // Check for timed out connections
        if (isConnectionTimedOut(connection)) {
            logger_->debug("Cleaning up timed out connection");
            connection->disconnect();
            it = connections_.erase(it);
            continue;
        }

        // Check for inactive connections
        if (!connection->isActive()) {
            logger_->debug("Cleaning up inactive connection");
            connection->disconnect();
            it = connections_.erase(it);
            continue;
        }

        ++it;
    }

    size_t final_count = connections_.size();
    if (final_count < initial_count) {
        logger_->info("Cleanup removed " + std::to_string(initial_count - final_count) +
                     " connections, remaining: " + std::to_string(final_count));
    }
}

std::map<std::string, size_t> FTPConnectionManager::getConnectionStats() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    std::map<std::string, size_t> stats;

    for (const auto& connection : connections_) {
        if (connection) {
            std::string username = connection->getUsername();
            if (username.empty()) {
                username = "anonymous";
            }

            stats[username]++;
        }
    }

    return stats;
}

std::map<std::string, size_t> FTPConnectionManager::getIPStats() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    std::map<std::string, size_t> stats;

    for (const auto& connection : connections_) {
        if (connection) {
            std::string ip = connection->getClientIP();
            stats[ip]++;
        }
    }

    return stats;
}

void FTPConnectionManager::getConnectionInfo(std::vector<ConnectionInfo>& info) const {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    info.clear();
    info.reserve(connections_.size());

    for (const auto& connection : connections_) {
        if (connection) {
            ConnectionInfo conn_info;
            conn_info.client_ip = connection->getClientIP();
            conn_info.username = connection->getUsername();
            conn_info.start_time = connection->getStartTime();
            conn_info.last_activity = connection->getLastActivity();
            conn_info.bytes_transferred = connection->getBytesTransferred();
            conn_info.commands_executed = connection->getCommandsExecuted();

            info.push_back(conn_info);
        }
    }
}

} // namespace ssftpd
