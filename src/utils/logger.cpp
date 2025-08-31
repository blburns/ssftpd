#include "ssftpd/logger.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <filesystem>

namespace ssftpd {

Logger::Logger(const std::string& log_file, LogLevel log_level, bool log_to_console, bool log_to_file)
    : log_file_(log_file)
    , log_level_(log_level)
    , log_format_(LogFormat::STANDARD)
    , log_to_console_(log_to_console)
    , log_to_file_(log_to_file)
    , custom_format_("")
    , max_log_size_(10 * 1024 * 1024) // 10MB
    , max_log_files_(5)
    , log_rotation_enabled_(true)
    , running_(true)
{
    if (log_to_file && !log_file_.empty()) {
        openLogFile();
    }
    
    // Start log rotation thread
    if (log_rotation_enabled_) {
        rotation_thread_ = std::thread(&Logger::rotationLoop, this);
    }
}

Logger::~Logger() {
    running_ = false;
    if (rotation_thread_.joinable()) {
        rotation_thread_.join();
    }
    
    if (log_stream_.is_open()) {
        log_stream_.close();
    }
}

void Logger::setLogFile(const std::string& log_file) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    if (log_stream_.is_open()) {
        log_stream_.close();
    }
    
    log_file_ = log_file;
    
    if (log_to_file_ && !log_file_.empty()) {
        openLogFile();
    }
}

void Logger::openLogFile() {
    try {
        // Create directory if it doesn't exist
        std::filesystem::path log_path(log_file_);
        std::filesystem::create_directories(log_path.parent_path());
        
        log_stream_.open(log_file_, std::ios::app);
        if (!log_stream_.is_open()) {
            std::cerr << "Failed to open log file: " << log_file_ << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error opening log file: " << e.what() << std::endl;
    }
}

std::string Logger::formatMessage(LogLevel level, const std::string& message, 
                                 const std::string& file, int line, const std::string& function) {
    std::ostringstream oss;
    
    switch (log_format_) {
        case LogFormat::SIMPLE:
            oss << "[" << getLevelString(level) << "] " << message;
            break;
            
        case LogFormat::STANDARD:
            oss << "[" << getCurrentTimestamp() << "] "
                << "[" << getLevelString(level) << "] "
                << "[" << getCurrentThreadId() << "] "
                << message;
            break;
            
        case LogFormat::EXTENDED:
            oss << "[" << getCurrentTimestamp() << "] "
                << "[" << getLevelString(level) << "] "
                << "[" << getCurrentThreadId() << "] "
                << "[" << file << ":" << line << "] "
                << "[" << function << "] "
                << message;
            break;
            
        case LogFormat::JSON:
            oss << "{"
                << "\"timestamp\":\"" << getCurrentTimestamp() << "\","
                << "\"level\":\"" << getLevelString(level) << "\","
                << "\"thread\":\"" << getCurrentThreadId() << "\","
                << "\"file\":\"" << file << ":" << line << "\","
                << "\"function\":\"" << function << "\","
                << "\"message\":\"" << escapeJsonString(message) << "\""
                << "}";
            break;
            
        case LogFormat::CUSTOM:
            oss << formatCustomMessage(level, message, file, line, function);
            break;
    }
    
    return oss.str();
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

std::string Logger::getCurrentThreadId() {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
}

std::string Logger::getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

std::string Logger::escapeJsonString(const std::string& str) {
    std::string result;
    result.reserve(str.length());
    
    for (char c : str) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b";  break;
            case '\f': result += "\\f";  break;
            case '\n': result += "\\n";  break;
            case '\r': result += "\\r";  break;
            case '\t': result += "\\t";  break;
            default:   result += c;      break;
        }
    }
    
    return result;
}

std::string Logger::formatCustomMessage(LogLevel level, const std::string& message,
                                       const std::string& file, int line, const std::string& function) {
    std::string result = custom_format_;
    
    // Replace placeholders
    replaceAll(result, "%timestamp%", getCurrentTimestamp());
    replaceAll(result, "%level%", getLevelString(level));
    replaceAll(result, "%thread%", getCurrentThreadId());
    replaceAll(result, "%file%", file + ":" + std::to_string(line));
    replaceAll(result, "%function%", function);
    replaceAll(result, "%message%", message);
    
    return result;
}

void Logger::replaceAll(std::string& str, const std::string& from, const std::string& to) {
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.length(), to);
        pos += to.length();
    }
}

void Logger::writeLog(LogLevel level, const std::string& message,
                      const std::string& file, int line, const std::string& function) {
    if (level < log_level_) {
        return;
    }
    
    std::string formatted_message = formatMessage(level, message, file, line, function);
    
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    // Console output
    if (log_to_console_) {
        if (level >= LogLevel::ERROR) {
            std::cerr << formatted_message << std::endl;
        } else {
            std::cout << formatted_message << std::endl;
        }
    }
    
    // File output
    if (log_to_file_ && log_stream_.is_open()) {
        log_stream_ << formatted_message << std::endl;
        log_stream_.flush();
        
        // Check if rotation is needed
        if (log_rotation_enabled_) {
            checkRotation();
        }
    }
}

void Logger::checkRotation() {
    if (log_stream_.is_open()) {
        log_stream_.seekp(0, std::ios::end);
        std::streampos file_size = log_stream_.tellp();
        
        if (file_size > static_cast<std::streampos>(max_log_size_)) {
            rotateLogFile();
        }
    }
}

void Logger::rotateLogFile() {
    if (log_file_.empty()) return;
    
    try {
        log_stream_.close();
        
        std::filesystem::path log_path(log_file_);
        std::string base_name = log_path.stem().string();
        std::string extension = log_path.extension().string();
        
        // Remove old rotated files
        for (int i = max_log_files_ - 1; i >= 0; --i) {
            std::string old_file = log_path.parent_path().string() + "/" + 
                                  base_name + "." + std::to_string(i) + extension;
            if (std::filesystem::exists(old_file)) {
                std::filesystem::remove(old_file);
            }
        }
        
        // Rename current log file
        std::string rotated_file = log_path.parent_path().string() + "/" + 
                                  base_name + ".1" + extension;
        std::filesystem::rename(log_file_, rotated_file);
        
        // Reopen log file
        openLogFile();
        
    } catch (const std::exception& e) {
        std::cerr << "Error during log rotation: " << e.what() << std::endl;
        // Try to reopen the original file
        openLogFile();
    }
}

void Logger::rotationLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(60)); // Check every minute
        
        if (log_rotation_enabled_ && log_to_file_ && !log_file_.empty()) {
            checkRotation();
        }
    }
}

// Logging methods
void Logger::trace(const std::string& message, const std::string& file, int line, const std::string& function) {
    writeLog(LogLevel::TRACE, message, file, line, function);
}

void Logger::debug(const std::string& message, const std::string& file, int line, const std::string& function) {
    writeLog(LogLevel::DEBUG, message, file, line, function);
}

void Logger::info(const std::string& message, const std::string& file, int line, const std::string& function) {
    writeLog(LogLevel::INFO, message, file, line, function);
}

void Logger::warn(const std::string& message, const std::string& file, int line, const std::string& function) {
    writeLog(LogLevel::WARN, message, file, line, function);
}

void Logger::error(const std::string& message, const std::string& file, int line, const std::string& function) {
    writeLog(LogLevel::ERROR, message, file, line, function);
}

void Logger::fatal(const std::string& message, const std::string& file, int line, const std::string& function) {
    writeLog(LogLevel::FATAL, message, file, line, function);
}

// Performance monitoring methods
void Logger::startTimer(const std::string& name) {
    std::lock_guard<std::mutex> lock(timer_mutex_);
    timers_[name] = std::chrono::steady_clock::now();
}

double Logger::endTimer(const std::string& name) {
    std::lock_guard<std::mutex> lock(timer_mutex_);
    
    auto it = timers_.find(name);
    if (it == timers_.end()) {
        return -1.0;
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - it->second);
    
    timers_.erase(it);
    
    return duration.count() / 1000.0; // Return milliseconds
}

std::string Logger::getPerformanceMetrics() {
    std::lock_guard<std::mutex> lock(timer_mutex_);
    
    if (timers_.empty()) {
        return "No active timers";
    }
    
    std::ostringstream oss;
    oss << "Active timers: ";
    for (const auto& timer : timers_) {
        oss << timer.first << " ";
    }
    
    return oss.str();
}

} // namespace ssftpd
