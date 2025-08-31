#include "ssftpd/ftp_server.hpp"
#include "ssftpd/logger.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>

namespace ssftpd {

FTPServer::FTPServer(std::shared_ptr<FTPServerConfig> config)
    : config_(config)
    , running_(false)
    , server_socket_(-1)
    , logger_(std::make_shared<Logger>())
    , connection_manager_(std::make_shared<FTPConnectionManager>(config, logger_))
    , user_manager_(std::make_shared<FTPUserManager>(config, logger_))
    , virtual_host_manager_(std::make_shared<FTPVirtualHostManager>(config, logger_))
    , statistics_(std::make_shared<FTPStatistics>())
    , rate_limiter_(std::make_shared<FTPRateLimiter>(config, logger_))
{
    if (!config_) {
        throw std::runtime_error("Configuration is required");
    }
    
    // Set up signal handlers
    setupSignalHandlers();
    
    // Initialize the server
    if (!initialize()) {
        throw std::runtime_error("Failed to initialize FTP server");
    }
}

FTPServer::~FTPServer() {
    stop();
}

bool FTPServer::initialize() {
    try {
        // Validate configuration
        if (!config_->validate()) {
            std::cerr << "Configuration validation failed:" << std::endl;
            for (const auto& error : config_->getErrors()) {
                std::cerr << "  ERROR: " << error << std::endl;
            }
            return false;
        }
        
        // Display warnings
        for (const auto& warning : config_->getWarnings()) {
            std::cout << "  WARNING: " << warning << std::endl;
        }
        
        // Initialize logger
        if (config_->enable_logging) {
            logger_->setLogFile(config_->logging.log_file);
            logger_->setLogLevel(parseLogLevel(config_->logging.log_level));
            logger_->setConsoleOutput(config_->logging.log_to_console);
        }
        
        // Initialize managers
        if (!user_manager_->initialize()) {
            logger_->error("Failed to initialize user manager");
            return false;
        }
        
        if (config_->enable_virtual_hosts && !virtual_host_manager_->initialize()) {
            logger_->error("Failed to initialize virtual host manager");
            return false;
        }
        
        if (config_->enable_rate_limiting && !rate_limiter_->initialize()) {
            logger_->error("Failed to initialize rate limiter");
            return false;
        }
        
        // Create server socket
        if (!createServerSocket()) {
            logger_->error("Failed to create server socket");
            return false;
        }
        
        logger_->info("FTP server initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Initialization error: " << e.what() << std::endl;
        return false;
    }
}

bool FTPServer::createServerSocket() {
    // Create socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ == -1) {
        logger_->error("Failed to create socket: " + std::string(strerror(errno)));
        return false;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        logger_->error("Failed to set SO_REUSEADDR: " + std::string(strerror(errno)));
        close(server_socket_);
        return false;
    }
    
    // Set non-blocking mode
    int flags = fcntl(server_socket_, F_GETFL, 0);
    if (flags == -1) {
        logger_->error("Failed to get socket flags: " + std::string(strerror(errno)));
        close(server_socket_);
        return false;
    }
    
    if (fcntl(server_socket_, F_SETFL, flags | O_NONBLOCK) == -1) {
        logger_->error("Failed to set non-blocking mode: " + std::string(strerror(errno)));
        close(server_socket_);
        return false;
    }
    
    // Bind socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config_->connection.bind_port);
    
    if (config_->connection.bind_address == "0.0.0.0") {
        server_addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, config_->connection.bind_address.c_str(), &server_addr.sin_addr) <= 0) {
            logger_->error("Invalid bind address: " + config_->connection.bind_address);
            close(server_socket_);
            return false;
        }
    }
    
    if (bind(server_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        logger_->error("Failed to bind socket: " + std::string(strerror(errno)));
        close(server_socket_);
        return false;
    }
    
    // Listen for connections
    if (listen(server_socket_, config_->connection.max_connections) < 0) {
        logger_->error("Failed to listen on socket: " + std::string(strerror(errno)));
        close(server_socket_);
        return false;
    }
    
    logger_->info("Server socket created successfully on " + 
                  config_->connection.bind_address + ":" + 
                  std::to_string(config_->connection.bind_port));
    
    return true;
}

bool FTPServer::start() {
    if (running_) {
        logger_->warn("Server is already running");
        return true;
    }
    
    if (server_socket_ == -1) {
        logger_->error("Server socket not initialized");
        return false;
    }
    
    running_ = true;
    logger_->info("Starting FTP server...");
    
    // Start connection manager
    if (!connection_manager_->start()) {
        logger_->error("Failed to start connection manager");
        running_ = false;
        return false;
    }
    
    // Start statistics collection if enabled
    if (config_->enable_statistics) {
        statistics_->start();
    }
    
    // Start monitoring if enabled
    if (config_->enable_monitoring) {
        startMonitoring();
    }
    
    // Main server loop
    try {
        mainLoop();
    } catch (const std::exception& e) {
        logger_->error("Server main loop error: " + std::string(e.what()));
        running_ = false;
        return false;
    }
    
    return true;
}

void FTPServer::mainLoop() {
    logger_->info("FTP server main loop started");
    
    while (running_) {
        // Accept new connections
        acceptConnections();
        
        // Process existing connections
        connection_manager_->processConnections();
        
        // Update statistics
        if (config_->enable_statistics) {
            statistics_->update();
        }
        
        // Sleep briefly to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    logger_->info("FTP server main loop stopped");
}

void FTPServer::acceptConnections() {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    while (running_) {
        int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_addr_len);
        
        if (client_socket == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No pending connections
                break;
            } else {
                logger_->error("Accept error: " + std::string(strerror(errno)));
                break;
            }
        }
        
        // Check rate limiting
        std::string client_ip = inet_ntoa(client_addr.sin_addr);
        if (config_->enable_rate_limiting && !rate_limiter_->allowConnection(client_ip)) {
            logger_->warn("Rate limit exceeded for client: " + client_ip);
            close(client_socket);
            continue;
        }
        
        // Check connection limit
        if (connection_manager_->getConnectionCount() >= config_->connection.max_connections) {
            logger_->warn("Connection limit reached, rejecting client: " + client_ip);
            close(client_socket);
            continue;
        }
        
        // Create new connection
        auto connection = std::make_shared<FTPConnection>(
            client_socket, client_addr, config_, logger_, user_manager_, virtual_host_manager_);
        
        if (connection_manager_->addConnection(connection)) {
            logger_->info("New connection accepted from " + client_ip);
            statistics_->incrementConnections();
        } else {
            logger_->error("Failed to add connection from " + client_ip);
            close(client_socket);
        }
    }
}

void FTPServer::stop() {
    if (!running_) {
        return;
    }
    
    logger_->info("Stopping FTP server...");
    running_ = false;
    
    // Stop connection manager
    if (connection_manager_) {
        connection_manager_->stop();
    }
    
    // Stop statistics
    if (config_->enable_statistics && statistics_) {
        statistics_->stop();
    }
    
    // Stop monitoring
    stopMonitoring();
    
    // Close server socket
    if (server_socket_ != -1) {
        close(server_socket_);
        server_socket_ = -1;
    }
    
    logger_->info("FTP server stopped");
}

bool FTPServer::isRunning() const {
    return running_;
}

void FTPServer::reloadConfiguration() {
    logger_->info("Reloading configuration...");
    
    // Stop the server temporarily
    bool was_running = running_;
    if (was_running) {
        stop();
    }
    
    // Reload configuration
    if (config_->loadFromFile(config_->config_file)) {
        logger_->info("Configuration reloaded successfully");
        
        // Reinitialize components
        if (initialize()) {
            if (was_running) {
                start();
            }
        } else {
            logger_->error("Failed to reinitialize server after configuration reload");
        }
    } else {
        logger_->error("Failed to reload configuration");
        // Try to restore previous state
        if (was_running) {
            initialize();
            start();
        }
    }
}

void FTPServer::setupSignalHandlers() {
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);
}

void FTPServer::signalHandler(int signal) {
    switch (signal) {
        case SIGINT:
        case SIGTERM:
            // Graceful shutdown
            if (instance_) {
                instance_->stop();
            }
            break;
        case SIGHUP:
            // Reload configuration
            if (instance_) {
                instance_->reloadConfiguration();
            }
            break;
    }
}

void FTPServer::startMonitoring() {
    if (monitoring_thread_.joinable()) {
        monitoring_thread_.join();
    }
    
    monitoring_thread_ = std::thread([this]() {
        while (running_ && config_->enable_monitoring) {
            // Monitor system resources
            monitorSystemResources();
            
            // Monitor connection health
            monitorConnections();
            
            // Sleep for monitoring interval
            std::this_thread::sleep_for(std::chrono::seconds(30));
        }
    });
}

void FTPServer::stopMonitoring() {
    if (monitoring_thread_.joinable()) {
        monitoring_thread_.join();
    }
}

void FTPServer::monitorSystemResources() {
    // Monitor memory usage
    if (config_->max_memory_usage > 0) {
        // This is a simplified memory check
        // In a real implementation, you would get actual memory usage
        size_t current_memory = 0; // Placeholder
        
        if (current_memory > config_->max_memory_usage) {
            logger_->warn("Memory usage limit exceeded: " + 
                         std::to_string(current_memory) + " bytes");
        }
    }
}

void FTPServer::monitorConnections() {
    if (connection_manager_) {
        size_t connection_count = connection_manager_->getConnectionCount();
        size_t max_connections = config_->connection.max_connections;
        
        if (connection_count > max_connections * 0.8) {
            logger_->warn("Connection count approaching limit: " + 
                         std::to_string(connection_count) + "/" + 
                         std::to_string(max_connections));
        }
    }
}

LogLevel FTPServer::parseLogLevel(const std::string& level_str) {
    if (level_str == "TRACE") return LogLevel::TRACE;
    if (level_str == "DEBUG") return LogLevel::DEBUG;
    if (level_str == "INFO") return LogLevel::INFO;
    if (level_str == "WARN") return LogLevel::WARN;
    if (level_str == "ERROR") return LogLevel::ERROR;
    if (level_str == "FATAL") return LogLevel::FATAL;
    
    // Default to INFO
    return LogLevel::INFO;
}

// Static instance pointer for signal handling
FTPServer* FTPServer::instance_ = nullptr;

} // namespace ssftpd
