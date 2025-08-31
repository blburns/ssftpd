#include "ssftpd/logger.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <filesystem>
#include <algorithm>

namespace ssftpd {

Logger::Logger(const std::string& log_file, LogLevel level, bool log_to_console, bool log_to_file)
    : log_file_(log_file)
    , log_level_(level)
    , log_format_(LogFormat::SIMPLE)
    , custom_format_("")
    , log_to_console_(log_to_console)
    , log_to_file_(log_to_file)
    , log_rotation_enabled_(false)
    , max_log_size_(10 * 1024 * 1024) // 10MB
    , max_log_files_(5)
    , performance_monitoring_(false)
    , log_filter_("")
    , log_buffer_size_(8192)
    , async_logging_(false)
    , messages_logged_(0)
    , bytes_written_(0)
    , files_rotated_(0)
    , start_time_(std::chrono::steady_clock::now())
    , total_log_time_(0)
    , max_log_time_(0)
    , min_log_time_(UINT64_MAX)
    , log_calls_(0)
    , async_running_(false)
{
    if (log_to_file && !log_file_.empty()) {
        // Create log directory if it doesn't exist
        auto log_dir = std::filesystem::path(log_file_).parent_path();
        if (!log_dir.empty() && !std::filesystem::exists(log_dir)) {
            std::filesystem::create_directories(log_dir);
        }
        
        // Open log file
        log_stream_.open(log_file_, std::ios::app);
    }
    
    if (async_logging_) {
        async_running_ = true;
        async_thread_ = std::thread(&Logger::asyncLoggingThread, this);
    }
}

Logger::~Logger() {
    if (async_logging_) {
        async_running_ = false;
        async_condition_.notify_all();
        if (async_thread_.joinable()) {
            async_thread_.join();
        }
    }
    
    if (log_stream_.is_open()) {
        log_stream_.close();
    }
}

void Logger::log(LogLevel level, const std::string& message, 
                 const std::string& file, int line, const std::string& function) {
    if (level < log_level_) {
        return;
    }
    
    if (!shouldLogMessage(message)) {
        return;
    }
    
    auto start_time = std::chrono::steady_clock::now();
    
    std::string formatted_message = formatMessage(level, message, file, line, function);
    
    if (log_to_console_) {
        writeToConsole(formatted_message);
    }
    
    if (log_to_file_ && log_stream_.is_open()) {
        writeToFile(formatted_message);
    }
    
    messages_logged_++;
    bytes_written_ += formatted_message.length();
    
    if (performance_monitoring_) {
        updatePerformanceMetrics(start_time);
    }
}

void Logger::trace(const std::string& message, const std::string& file, int line, const std::string& function) {
    log(LogLevel::TRACE, message, file, line, function);
}

void Logger::debug(const std::string& message, const std::string& file, int line, const std::string& function) {
    log(LogLevel::DEBUG, message, file, line, function);
}

void Logger::info(const std::string& message, const std::string& file, int line, const std::string& function) {
    log(LogLevel::INFO, message, file, line, function);
}

void Logger::warn(const std::string& message, const std::string& file, int line, const std::string& function) {
    log(LogLevel::WARN, message, file, line, function);
}

void Logger::error(const std::string& message, const std::string& file, int line, const std::string& function) {
    log(LogLevel::ERROR, message, file, line, function);
}

void Logger::fatal(const std::string& message, const std::string& file, int line, const std::string& function) {
    log(LogLevel::FATAL, message, file, line, function);
}

std::string Logger::formatMessage(LogLevel level, const std::string& message, 
                                 const std::string& file, int line, const std::string& function) {
    std::ostringstream oss;
    
    switch (log_format_) {
        case LogFormat::SIMPLE:
            oss << "[" << getLogLevelString(level) << "] " << message;
            break;
            
        case LogFormat::STANDARD:
            oss << getCurrentTimestamp() << " [" << getLogLevelString(level) << "] "
                << message;
            break;
            
        case LogFormat::EXTENDED:
            oss << getCurrentTimestamp() << " [" << getLogLevelString(level) << "] "
                << "[" << getCurrentThreadId() << "] "
                << message;
            break;
            
        case LogFormat::JSON:
            oss << "{"
                << "\"timestamp\":\"" << getCurrentTimestamp() << "\","
                << "\"level\":\"" << getLogLevelString(level) << "\","
                << "\"thread\":\"" << getCurrentThreadId() << "\","
                << "\"message\":\"" << escapeJsonString(message) << "\"";
            
            if (!file.empty()) {
                oss << ",\"file\":\"" << escapeJsonString(file) << "\""
                    << ",\"line\":" << line
                    << ",\"function\":\"" << escapeJsonString(function) << "\"";
            }
            
            oss << "}";
            break;
            
        case LogFormat::CUSTOM:
            oss << formatCustomMessage(level, message, file, line, function);
            break;
            
        default:
            oss << "[" << getLogLevelString(level) << "] " << message;
            break;
    }
    
    return oss.str();
}

void Logger::writeToConsole(const std::string& formatted_message) {
    std::cout << formatted_message << std::endl;
}

void Logger::writeToFile(const std::string& formatted_message) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    if (log_stream_.is_open()) {
        log_stream_ << formatted_message << std::endl;
        log_stream_.flush();
        
        // Check if log rotation is needed
        if (log_rotation_enabled_ && shouldRotateLog()) {
            rotateLog();
        }
    }
}

bool Logger::shouldRotateLog() const {
    if (!log_stream_.is_open()) {
        return false;
    }
    
    // Use const_cast to access non-const method on const object
    auto& non_const_stream = const_cast<std::ofstream&>(log_stream_);
    auto current_pos = non_const_stream.tellp();
    return current_pos > 0 && static_cast<size_t>(current_pos) > max_log_size_;
}

void Logger::rotateLog() {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    if (log_stream_.is_open()) {
        log_stream_.close();
    }
    
    // Rotate existing log files
    for (size_t i = max_log_files_ - 1; i > 0; --i) {
        std::string old_name = log_file_ + "." + std::to_string(i);
        std::string new_name = log_file_ + "." + std::to_string(i + 1);
        
        if (std::filesystem::exists(old_name)) {
            if (i == max_log_files_ - 1) {
                std::filesystem::remove(old_name);
            } else {
                std::filesystem::rename(old_name, new_name);
            }
        }
    }
    
    // Rename current log file
    if (std::filesystem::exists(log_file_)) {
        std::filesystem::rename(log_file_, log_file_ + ".1");
    }
    
    // Open new log file
    log_stream_.open(log_file_, std::ios::app);
    files_rotated_++;
}

std::string Logger::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}

std::string Logger::getCurrentThreadId() const {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
}

std::string Logger::getLogLevelString(LogLevel level) const {
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

std::string Logger::getLogLevelColor(LogLevel level) const {
    switch (level) {
        case LogLevel::TRACE: return "\033[36m"; // Cyan
        case LogLevel::DEBUG: return "\033[35m"; // Magenta
        case LogLevel::INFO:  return "\033[32m"; // Green
        case LogLevel::WARN:  return "\033[33m"; // Yellow
        case LogLevel::ERROR: return "\033[31m"; // Red
        case LogLevel::FATAL: return "\033[1;31m"; // Bold Red
        default: return "\033[0m"; // Reset
    }
}

std::string Logger::resetConsoleColor() const {
    return "\033[0m";
}

void Logger::asyncLoggingThread() {
    while (async_running_) {
        std::unique_lock<std::mutex> lock(async_mutex_);
        async_condition_.wait_for(lock, std::chrono::milliseconds(100));
        
        processAsyncBuffer();
    }
}

void Logger::processAsyncBuffer() {
    std::vector<std::string> buffer_copy;
    {
        std::lock_guard<std::mutex> lock(async_mutex_);
        buffer_copy.swap(async_buffer_);
    }
    
    for (const auto& message : buffer_copy) {
        writeToFile(message);
    }
}

void Logger::addToAsyncBuffer(const std::string& message) {
    if (async_logging_) {
        std::lock_guard<std::mutex> lock(async_mutex_);
        async_buffer_.push_back(message);
        async_condition_.notify_one();
    }
}

bool Logger::messageMatchesFilter(const std::string& message) const {
    if (log_filter_.empty()) {
        return true;
    }
    
    return message.find(log_filter_) != std::string::npos;
}

void Logger::updatePerformanceMetrics(const std::chrono::steady_clock::time_point& start_time) {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    auto duration_us = duration.count();
    
    total_log_time_ += duration_us;
    
    // Handle atomic min/max operations properly
    uint64_t current_max = max_log_time_.load();
    while (static_cast<uint64_t>(duration_us) > current_max && 
           !max_log_time_.compare_exchange_weak(current_max, static_cast<uint64_t>(duration_us))) {
        // Loop until we successfully update the max value
    }
    
    uint64_t current_min = min_log_time_.load();
    while (static_cast<uint64_t>(duration_us) < current_min && 
           !min_log_time_.compare_exchange_weak(current_min, static_cast<uint64_t>(duration_us))) {
        // Loop until we successfully update the min value
    }
    
    log_calls_++;
}

void Logger::flush() {
    if (log_stream_.is_open()) {
        log_stream_.flush();
    }
}

void Logger::close() {
    if (log_stream_.is_open()) {
        log_stream_.close();
    }
}

bool Logger::isLogFileOpen() const {
    return log_stream_.is_open();
}

std::string Logger::getStatistics() const {
    std::ostringstream oss;
    oss << "Messages logged: " << messages_logged_.load() << "\n"
        << "Bytes written: " << bytes_written_.load() << "\n"
        << "Files rotated: " << files_rotated_.load() << "\n"
        << "Log calls: " << log_calls_.load();
    
    if (performance_monitoring_) {
        oss << "\nTotal log time: " << total_log_time_.load() << " μs"
            << "\nMax log time: " << max_log_time_.load() << " μs"
            << "\nMin log time: " << min_log_time_.load() << " μs"
            << "\nAverage log time: " << (log_calls_.load() > 0 ? total_log_time_.load() / log_calls_.load() : 0) << " μs";
    }
    
    return oss.str();
}

void Logger::resetStatistics() {
    messages_logged_ = 0;
    bytes_written_ = 0;
    files_rotated_ = 0;
    total_log_time_ = 0;
    max_log_time_ = 0;
    min_log_time_ = UINT64_MAX;
    log_calls_ = 0;
    start_time_ = std::chrono::steady_clock::now();
}

std::string Logger::getPerformanceMetrics() const {
    if (!performance_monitoring_) {
        return "Performance monitoring is disabled";
    }
    
    std::ostringstream oss;
    oss << "Total log time: " << total_log_time_.load() << " μs\n"
        << "Max log time: " << max_log_time_.load() << " μs\n"
        << "Min log time: " << min_log_time_.load() << " μs\n"
        << "Log calls: " << log_calls_.load() << "\n"
        << "Average log time: " << (log_calls_.load() > 0 ? total_log_time_.load() / log_calls_.load() : 0) << " μs";
    
    return oss.str();
}

void Logger::setLogFilter(const std::string& filter) {
    log_filter_ = filter;
}

bool Logger::shouldLogMessage(const std::string& message) const {
    return messageMatchesFilter(message);
}

void Logger::setLogBufferSize(size_t buffer_size) {
    log_buffer_size_ = buffer_size;
}

void Logger::setLogFile(const std::string& log_file) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    if (log_stream_.is_open()) {
        log_stream_.close();
    }
    
    log_file_ = log_file;
    
    if (log_to_file_ && !log_file_.empty()) {
        // Create log directory if it doesn't exist
        auto log_dir = std::filesystem::path(log_file_).parent_path();
        if (!log_dir.empty() && !std::filesystem::exists(log_dir)) {
            std::filesystem::create_directories(log_dir);
        }
        
        // Open new log file
        log_stream_.open(log_file_, std::ios::app);
    }
}

void Logger::setAsyncLogging(bool enable) {
    if (enable != async_logging_) {
        async_logging_ = enable;
        
        if (enable) {
            async_running_ = true;
            async_thread_ = std::thread(&Logger::asyncLoggingThread, this);
        } else {
            async_running_ = false;
            async_condition_.notify_all();
            if (async_thread_.joinable()) {
                async_thread_.join();
            }
        }
    }
}

std::string Logger::formatCustomMessage(LogLevel level, const std::string& message, 
                                       const std::string& file, int line, const std::string& function) {
    // Simple custom format implementation
    std::ostringstream oss;
    oss << "CUSTOM[" << getLogLevelString(level) << "] " << message;
    if (!file.empty()) {
        oss << " (" << file << ":" << line << " in " << function << ")";
    }
    return oss.str();
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

} // namespace ssftpd
