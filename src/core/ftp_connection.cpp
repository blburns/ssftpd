#include "ssftpd/ftp_connection.hpp"
#include "ssftpd/logger.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <chrono>

namespace ssftpd {

FTPConnection::FTPConnection(socket_t client_socket, 
                            const std::string& client_addr,
                            std::shared_ptr<FTPVirtualHost> virtual_host)
    : client_socket_(client_socket)
    , client_addr_(client_addr)
    , virtual_host_(virtual_host)
    , active_(true)
    , state_(FTPConnectionState::CONNECTED)
    , current_directory_("/")
    , transfer_type_(FTPTransferType::ASCII)
    , transfer_mode_(FTPTransferMode::STREAM)
    , passive_mode_(false)
    , data_port_(0)
    , data_socket_(-1)
    , data_socket_port_(0)
    , start_time_(std::chrono::steady_clock::now())
    , last_activity_time_(std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count()))
    , bytes_sent_(0)
    , bytes_received_(0)
    , files_sent_(0)
    , files_received_(0)
    , logger_(std::make_shared<Logger>())
{
    // Set socket to non-blocking mode
    int flags = fcntl(client_socket_, F_GETFL, 0);
    fcntl(client_socket_, F_SETFL, flags | O_NONBLOCK);
    
    // Send welcome message
    if (virtual_host_) {
        logger_->info("New FTP connection from " + client_addr_ + " to virtual host: " + virtual_host_->getHostname());
    } else {
        logger_->info("New FTP connection from " + client_addr_);
    }
}

FTPConnection::~FTPConnection() {
    disconnect();
}

void FTPConnection::start() {
    if (!active_.load()) {
        return;
    }
    
    // Start connection thread
    connection_thread_ = std::thread(&FTPConnection::connectionLoop, this);
}

void FTPConnection::stop() {
    active_.store(false);
    
    if (connection_thread_.joinable()) {
        connection_thread_.join();
    }
    
    disconnect();
}

void FTPConnection::connectionLoop() {
    while (active_.load()) {
        // Read command from client
        std::string command = readCommand();
        if (!command.empty()) {
            handleCommand(command);
            updateActivityTime();
        }
        
        // Sleep briefly to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

std::string FTPConnection::readCommand() {
    char buffer[1024];
    ssize_t bytes_read = recv(client_socket_, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            // Client disconnected
            logger_->info("Client disconnected: " + client_addr_);
            disconnect();
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            // Error reading
            logger_->error("Error reading from client: " + std::string(strerror(errno)));
            disconnect();
        }
        return "";
    }
    
    buffer[bytes_read] = '\0';
    
    // Remove trailing whitespace and newlines
    std::string command(buffer);
    command.erase(command.find_last_not_of(" \r\n\t") + 1);
    
    return command;
}

void FTPConnection::handleCommand(const std::string& command) {
    if (command.empty()) {
        return;
    }
    
    logger_->debug("Command from " + client_addr_ + ": " + command);
    
    // Parse command line
    auto args = parseCommandLine(command);
    if (args.empty()) {
        return;
    }
    
    std::string cmd = args[0];
    // Convert to uppercase for comparison
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    
    // Execute command
    if (cmd == "USER") {
        handleUSER(args);
    } else if (cmd == "PASS") {
        handlePASS(args);
    } else if (cmd == "QUIT") {
        handleQUIT(args);
    } else if (cmd == "PWD") {
        handlePWD(args);
    } else if (cmd == "CWD") {
        handleCWD(args);
    } else if (cmd == "LIST") {
        handleLIST(args);
    } else if (cmd == "NOOP") {
        handleNOOP(args);
    } else {
        // Unknown command
        sendResponse(500, "Unknown command: " + cmd);
    }
}

std::vector<std::string> FTPConnection::parseCommandLine(const std::string& line) {
    std::vector<std::string> args;
    std::istringstream iss(line);
    std::string arg;
    
    while (iss >> arg) {
        args.push_back(arg);
    }
    
    return args;
}

void FTPConnection::handleUSER(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        sendResponse(501, "Syntax error in parameters or arguments.");
        return;
    }
    
    std::string username = args[1];
    username_buffer_ = username;
    
    if (username == "anonymous") {
        sendResponse(331, "User " + username + " OK. Password required.");
    } else {
        sendResponse(331, "User " + username + " OK. Password required.");
    }
}

void FTPConnection::handlePASS(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        sendResponse(501, "Syntax error in parameters or arguments.");
        return;
    }
    
    if (username_buffer_.empty()) {
        sendResponse(503, "Login with USER first.");
        return;
    }
    
    std::string password = args[1];
    
    // Simple authentication (in production, implement proper authentication)
    if (username_buffer_ == "admin" && password == "admin") {
        state_ = FTPConnectionState::AUTHENTICATED;
        sendResponse(230, "User " + username_buffer_ + " logged in.");
        logger_->info("User " + username_buffer_ + " authenticated from " + client_addr_);
    } else {
        sendResponse(530, "Login incorrect.");
        logger_->warn("Failed login attempt for user " + username_buffer_ + " from " + client_addr_);
    }
}

void FTPConnection::handleQUIT(const std::vector<std::string>& args) {
    (void)args; // Suppress unused parameter warning
    sendResponse(221, "Goodbye");
    disconnect();
}

void FTPConnection::handleSYST(const std::vector<std::string>& args) {
    (void)args; // Suppress unused parameter warning
    sendResponse(215, "UNIX Type: L8");
}

void FTPConnection::handleFEAT(const std::vector<std::string>& args) {
    (void)args; // Suppress unused parameter warning
    sendResponse(211, "Features:");
    sendResponse(211, " UTF8");
    sendResponse(211, " PASV");
    sendResponse(211, " EPSV");
    sendResponse(211, " REST STREAM");
    sendResponse(211, " SIZE");
    sendResponse(211, " MDTM");
    sendResponse(211, " End");
}

void FTPConnection::handlePWD(const std::vector<std::string>& args) {
    (void)args; // Suppress unused parameter warning
    std::string response = "257 \"" + current_directory_ + "\" is current directory";
    sendResponse(257, response);
}

void FTPConnection::handleCWD(const std::vector<std::string>& args) {
    if (state_ != FTPConnectionState::AUTHENTICATED) {
        sendResponse(530, "Please login with USER and PASS.");
        return;
    }
    
    if (args.size() < 2) {
        sendResponse(501, "Syntax error in parameters or arguments.");
        return;
    }
    
    std::string directory = args[1];
    
    // Simple directory change (in production, validate path)
    if (directory == "/" || directory.empty()) {
        current_directory_ = "/";
        sendResponse(250, "Directory changed to /");
    } else {
        current_directory_ = directory;
        sendResponse(250, "Directory changed to " + directory);
    }
}

void FTPConnection::handleLIST(const std::vector<std::string>& args) {
    (void)args; // Suppress unused parameter warning
    // Simple directory listing (in production, implement proper directory listing)
    std::string listing = "drwxr-xr-x 2 user group 4096 Jan 1 00:00 .\r\n"
                          "drwxr-xr-x 2 user group 4096 Jan 1 00:00 ..\r\n";
    
    sendResponse(150, "Here comes the directory listing");
    sendData(listing.c_str(), listing.length());
    sendResponse(226, "Directory send OK");
}

void FTPConnection::handleNOOP(const std::vector<std::string>& args) {
    (void)args; // Suppress unused parameter warning
    sendResponse(200, "OK");
}

void FTPConnection::updateActivityTime() {
    last_activity_time_ = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

bool FTPConnection::sendData(const char* data, size_t length) {
    if (!client_socket_ || !data || length == 0) {
        return false;
    }
    
    ssize_t bytes_sent = send(client_socket_, data, length, 0);
    if (bytes_sent > 0) {
        bytes_sent_ += bytes_sent;
        return true;
    }
    
    return false;
}

void FTPConnection::sendResponse(int code, const std::string& message) {
    std::string response = std::to_string(code) + " " + message + "\r\n";
    send(client_socket_, response.c_str(), response.length(), 0);
}

void FTPConnection::disconnect() {
    active_ = false;
    
    if (client_socket_ != INVALID_SOCKET) {
        close(client_socket_);
        client_socket_ = INVALID_SOCKET;
    }
    
    if (data_socket_ != INVALID_SOCKET) {
        close(data_socket_);
        data_socket_ = INVALID_SOCKET;
    }
}

void FTPConnection::process() {
    if (!active_.load()) {
        return;
    }
    
    try {
        std::string command = readCommand();
        if (!command.empty()) {
            handleCommand(command);
        }
    } catch (const std::exception& e) {
        logger_->error("Error processing connection: " + std::string(e.what()));
        disconnect();
    }
}

void FTPConnection::setStartTime(const std::chrono::steady_clock::time_point& start_time) {
    start_time_ = start_time;
}

std::string FTPConnection::getUsername() const {
    return username_;
}

std::chrono::steady_clock::time_point FTPConnection::getLastActivity() const {
    return start_time_; // Use start_time_ as the last activity time
}

uint64_t FTPConnection::getBytesTransferred() const {
    return bytes_sent_.load() + bytes_received_.load();
}

uint64_t FTPConnection::getCommandsExecuted() const {
    return files_sent_.load() + files_received_.load();
}

} // namespace ssftpd
