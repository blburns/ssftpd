#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <signal.h>
#include <csignal>
#include "ssftpd/ftp_server.hpp"
#include "ssftpd/ftp_server_config.hpp"
#include "ssftpd/logger.hpp"

using namespace ssftpd;

// Global variables for signal handling
std::shared_ptr<FTPServer> g_server;
std::shared_ptr<Logger> g_logger;
std::atomic<bool> g_shutdown_requested(false);

/**
 * @brief Signal handler for graceful shutdown
 * @param signal Signal number
 */
void signalHandler(int signal) {
    if (g_shutdown_requested.exchange(true)) {
        // Already shutting down, force exit
        std::exit(1);
    }
    
    g_logger->info("Received signal " + std::to_string(signal) + ", initiating graceful shutdown");
    
    if (g_server) {
        g_server->stop();
    }
}

/**
 * @brief Print usage information
 */
void printUsage() {
    std::cout << "\nUsage: ssftpd [OPTIONS] [COMMAND] [ARGS...]" << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  --help, -h           Show this help message" << std::endl;
    std::cout << "  --version, -v        Show version information" << std::endl;
    std::cout << "  --config, -c FILE    Use specified configuration file" << std::endl;
    std::cout << "  --verbose, -V        Enable verbose logging" << std::endl;
    std::cout << "  --daemon, -d         Run as daemon" << std::endl;
    std::cout << "  --foreground, -f     Run in foreground" << std::endl;
    std::cout << "  --test-config        Test configuration file" << std::endl;
    std::cout << "  --validate           Validate configuration" << std::endl;
    
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  start                Start the FTP server" << std::endl;
    std::cout << "  stop                 Stop the FTP server" << std::endl;
    std::cout << "  restart              Restart the FTP server" << std::endl;
    std::cout << "  status               Show server status" << std::endl;
    std::cout << "  reload               Reload configuration" << std::endl;
    std::cout << "  test                 Test server configuration" << std::endl;
    std::cout << "  user                 Manage users" << std::endl;
    std::cout << "  virtual              Manage virtual hosts" << std::endl;
    std::cout << "  ssl                  Manage SSL certificates" << std::endl;
    
    std::cout << "\nUser Subcommands:" << std::endl;
    std::cout << "  add                  Add new user" << std::endl;
    std::cout << "  remove               Remove user" << std::endl;
    std::cout << "  modify               Modify user" << std::endl;
    std::cout << "  list                 List users" << std::endl;
    std::cout << "  password             Change user password" << std::endl;
    
    std::cout << "\nVirtual Host Subcommands:" << std::endl;
    std::cout << "  add                  Add new virtual host" << std::endl;
    std::cout << "  remove               Remove virtual host" << std::endl;
    std::cout << "  modify               Modify virtual host" << std::endl;
    std::cout << "  list                 List virtual hosts" << std::endl;
    std::cout << "  enable               Enable virtual host" << std::endl;
    std::cout << "  disable              Disable virtual host" << std::endl;
    
    std::cout << "\nSSL Subcommands:" << std::endl;
    std::cout << "  generate             Generate self-signed certificate" << std::endl;
    std::cout << "  install              Install certificate" << std::endl;
    std::cout << "  renew                Renew certificate" << std::endl;
    std::cout << "  status               Show SSL status" << std::endl;
    
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  ssftpd start --config /etc/ssftpd/config.json" << std::endl;
    std::cout << "  ssftpd user add --username john --password secret --home /home/john" << std::endl;
    std::cout << "  ssftpd virtual add --hostname ftp.example.com --root /var/ftp/example" << std::endl;
    std::cout << "  ssftpd ssl generate --hostname ftp.example.com" << std::endl;
    std::cout << "  ssftpd --daemon start" << std::endl;
}

/**
 * @brief Print version information
 */
void printVersion() {
    std::cout << "ssftpd v0.1.0" << std::endl;
    std::cout << "Simple FTP Daemon for Linux, macOS, and Windows" << std::endl;
    std::cout << "Copyright (c) 2024 SimpleDaemons" << std::endl;
}

/**
 * @brief Parse command line arguments
 * @param argc Argument count
 * @param argv Argument vector
 * @param config_file Output configuration file path
 * @param command Output command
 * @param args Output command arguments
 * @param daemon_mode Output daemon mode flag
 * @param foreground_mode Output foreground mode flag
 * @param verbose Output verbose flag
 * @return true if parsed successfully, false otherwise
 */
bool parseArguments(int argc, char* argv[], 
                   std::string& config_file,
                   std::string& command,
                   std::vector<std::string>& args,
                   bool& daemon_mode,
                   bool& foreground_mode,
                   bool& verbose) {
    config_file.clear();
    command.clear();
    args.clear();
    daemon_mode = false;
    foreground_mode = false;
    verbose = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printUsage();
            return false;
        } else if (arg == "--version" || arg == "-v") {
            printVersion();
            return false;
        } else if (arg == "--config" || arg == "-c") {
            if (i + 1 < argc) {
                config_file = argv[++i];
            } else {
                std::cerr << "Error: --config requires a file path" << std::endl;
                return false;
            }
        } else if (arg == "--verbose" || arg == "-V") {
            verbose = true;
        } else if (arg == "--daemon" || arg == "-d") {
            daemon_mode = true;
        } else if (arg == "--foreground" || arg == "-f") {
            foreground_mode = true;
        } else if (arg == "--test-config") {
            command = "test-config";
        } else if (arg == "--validate") {
            command = "validate";
        } else if (arg[0] != '-') {
            // This is a command
            if (command.empty()) {
                command = arg;
            } else {
                args.push_back(arg);
            }
        } else {
            std::cerr << "Error: Unknown option: " << arg << std::endl;
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Setup signal handlers
 */
void setupSignalHandlers() {
    signal(SIGINT, signalHandler);   // Ctrl+C
    signal(SIGTERM, signalHandler);  // Termination request
    signal(SIGHUP, signalHandler);   // Hangup (reload config)
    
    #ifndef _WIN32
    signal(SIGUSR1, signalHandler);  // User defined signal 1
    signal(SIGUSR2, signalHandler);  // User defined signal 2
    #endif
}

/**
 * @brief Daemonize the process
 * @return true if successful, false otherwise
 */
bool daemonize() {
    #ifdef _WIN32
    // Windows doesn't support daemonization
    return false;
    #else
    // Fork the process
    pid_t pid = fork();
    if (pid < 0) {
        return false;
    }
    
    if (pid > 0) {
        // Parent process, exit
        exit(0);
    }
    
    // Child process continues
    // Create new session
    if (setsid() < 0) {
        return false;
    }
    
    // Change working directory
    if (chdir("/") < 0) {
        return false;
    }
    
    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    // Redirect to /dev/null
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);
    
    return true;
    #endif
}

/**
 * @brief Test configuration file
 * @param config_file Configuration file path
 * @return true if valid, false otherwise
 */
bool testConfiguration(const std::string& config_file) {
    auto config = std::make_shared<FTPServerConfig>();
    
    if (!config->loadFromFile(config_file)) {
        std::cerr << "Error: Failed to load configuration file: " << config_file << std::endl;
        return false;
    }
    
    if (!config->validate()) {
        std::cerr << "Error: Configuration validation failed:" << std::endl;
        for (const auto& error : config->getErrors()) {
            std::cerr << "  " << error << std::endl;
        }
        return false;
    }
    
    std::cout << "Configuration file is valid: " << config_file << std::endl;
    return true;
}

/**
 * @brief Validate configuration
 * @param config_file Configuration file path
 * @return true if valid, false otherwise
 */
bool validateConfiguration(const std::string& config_file) {
    auto config = std::make_shared<FTPServerConfig>();
    
    if (!config->loadFromFile(config_file)) {
        std::cerr << "Error: Failed to load configuration file: " << config_file << std::endl;
        return false;
    }
    
    std::cout << "Configuration validation results:" << std::endl;
    std::cout << "  File: " << config_file << std::endl;
    std::cout << "  Loaded: " << (config->getErrors().empty() ? "Yes" : "No") << std::endl;
    
    if (!config->getErrors().empty()) {
        std::cout << "  Errors:" << std::endl;
        for (const auto& error : config->getErrors()) {
            std::cout << "    " << error << std::endl;
        }
    }
    
    if (!config->getWarnings().empty()) {
        std::cout << "  Warnings:" << std::endl;
        for (const auto& warning : config->getWarnings()) {
            std::cout << "    " << warning << std::endl;
        }
    }
    
    return config->getErrors().empty();
}

/**
 * @brief Start the FTP server
 * @param config_file Configuration file path
 * @param daemon_mode Run as daemon
 * @return true if started successfully, false otherwise
 */
bool startServer(const std::string& config_file, bool daemon_mode) {
    // Load configuration
    auto config = std::make_shared<FTPServerConfig>();
    if (!config->loadFromFile(config_file)) {
        std::cerr << "Error: Failed to load configuration file: " << config_file << std::endl;
        return false;
    }
    
    if (!config->validate()) {
        std::cerr << "Error: Configuration validation failed:" << std::endl;
        for (const auto& error : config->getErrors()) {
            std::cerr << "  " << error << std::endl;
        }
        return false;
    }
    
    // Initialize logger
    g_logger = std::make_shared<Logger>(
        config->logging.log_file,
        config->logging.log_level == "TRACE" ? LogLevel::TRACE :
        config->logging.log_level == "DEBUG" ? LogLevel::DEBUG :
        config->logging.log_level == "INFO" ? LogLevel::INFO :
        config->logging.log_level == "WARN" ? LogLevel::WARN :
        config->logging.log_level == "ERROR" ? LogLevel::ERROR :
        config->logging.log_level == "FATAL" ? LogLevel::FATAL : LogLevel::INFO,
        config->logging.log_to_console,
        config->logging.log_to_file
    );
    
    g_logger->info("Starting Simple FTP Daemon v0.1.0");
    g_logger->info("Configuration file: " + config_file);
    
    // Create and start server
    g_server = std::make_shared<FTPServer>(config);
    
    if (!g_server->start()) {
        g_logger->error("Failed to start FTP server");
        return false;
    }
    
    g_logger->info("FTP server started successfully");
    g_logger->info("Listening on " + config->connection.bind_address + ":" + std::to_string(config->connection.bind_port));
    
    // Main server loop
    while (!g_shutdown_requested && g_server->isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    g_logger->info("FTP server shutdown complete");
    return true;
}

/**
 * @brief Main function
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code
 */
int main(int argc, char* argv[]) {
    // Parse command line arguments
    std::string config_file;
    std::string command;
    std::vector<std::string> args;
    bool daemon_mode = false;
    bool foreground_mode = false;
    bool verbose = false;
    
    if (!parseArguments(argc, argv, config_file, command, args, daemon_mode, foreground_mode, verbose)) {
        return 1;
    }
    
    // Set default configuration file if none specified
    if (config_file.empty()) {
        #ifdef _WIN32
        config_file = "C:\\Program Files\\ssftpd\\config\\ssftpd.conf";
        #else
        config_file = "/etc/ssftpd/ssftpd.conf";
        #endif
    }
    
    // Handle special commands
    if (command == "test-config") {
        return testConfiguration(config_file) ? 0 : 1;
    }
    
    if (command == "validate") {
        return validateConfiguration(config_file) ? 0 : 1;
    }
    
    // Setup signal handlers
    setupSignalHandlers();
    
    // Handle daemon mode
    if (daemon_mode && !foreground_mode) {
        if (!daemonize()) {
            std::cerr << "Error: Failed to daemonize process" << std::endl;
            return 1;
        }
    }
    
    // Start server if no specific command
    if (command.empty() || command == "start") {
        if (!startServer(config_file, daemon_mode)) {
            return 1;
        }
    } else {
        // Handle other commands (user management, virtual hosts, etc.)
        std::cout << "Command '" << command << "' not yet implemented" << std::endl;
        return 1;
    }
    
    return 0;
}
