#include "ssftpd/ftp_statistics.hpp"
#include "ssftpd/logger.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace ssftpd {

FTPStatistics::FTPStatistics()
    : running_(false)
    , start_time_(std::chrono::steady_clock::now())
    , total_connections_(0)
    , current_connections_(0)
    , total_requests_(0)
    , total_bytes_transferred_(0)
    , total_files_transferred_(0)
    , successful_logins_(0)
    , failed_logins_(0)
    , total_errors_(0)
{
}

FTPStatistics::~FTPStatistics() {
    stop();
}

void FTPStatistics::start() {
    if (running_) {
        return;
    }
    
    running_ = true;
    start_time_ = std::chrono::steady_clock::now();
    
    // Reset counters
    total_connections_ = 0;
    current_connections_ = 0;
    total_requests_ = 0;
    total_bytes_transferred_ = 0;
    total_files_transferred_ = 0;
    successful_logins_ = 0;
    failed_logins_ = 0;
    total_errors_ = 0;
}

void FTPStatistics::stop() {
    running_ = false;
}

void FTPStatistics::update() {
    if (!running_) {
        return;
    }
    
    // Update uptime and other time-based statistics
    auto now = std::chrono::steady_clock::now();
    uptime_ = now - start_time_;
}

void FTPStatistics::incrementConnections() {
    total_connections_++;
    current_connections_++;
}

void FTPStatistics::decrementConnections() {
    if (current_connections_ > 0) {
        current_connections_--;
    }
}

void FTPStatistics::incrementRequests() {
    total_requests_++;
}

void FTPStatistics::addBytesTransferred(size_t bytes) {
    total_bytes_transferred_ += bytes;
}

void FTPStatistics::incrementFilesTransferred() {
    total_files_transferred_++;
}

void FTPStatistics::incrementSuccessfulLogins() {
    successful_logins_++;
}

void FTPStatistics::incrementFailedLogins() {
    failed_logins_++;
}

void FTPStatistics::incrementErrors() {
    total_errors_++;
}

void FTPStatistics::setCurrentConnections(size_t count) {
    current_connections_ = count;
}

std::string FTPStatistics::getUptimeString() const {
    auto duration = uptime_;
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
    duration -= hours;
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
    duration -= minutes;
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << hours.count() << ":"
        << std::setfill('0') << std::setw(2) << minutes.count() << ":"
        << std::setfill('0') << std::setw(2) << seconds.count();
    
    return oss.str();
}

std::string FTPStatistics::getFormattedBytes(size_t bytes) const {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return oss.str();
}

std::string FTPStatistics::getSummary() const {
    std::ostringstream oss;
    
    oss << "FTP Server Statistics" << std::endl;
    oss << "====================" << std::endl;
    oss << "Uptime: " << getUptimeString() << std::endl;
    oss << "Total Connections: " << total_connections_ << std::endl;
    oss << "Current Connections: " << current_connections_ << std::endl;
    oss << "Total Requests: " << total_requests_ << std::endl;
    oss << "Total Bytes Transferred: " << getFormattedBytes(total_bytes_transferred_) << std::endl;
    oss << "Total Files Transferred: " << total_files_transferred_ << std::endl;
    oss << "Successful Logins: " << successful_logins_ << std::endl;
    oss << "Failed Logins: " << failed_logins_ << std::endl;
    oss << "Total Errors: " << total_errors_ << std::endl;
    
    if (uptime_.count() > 0) {
        auto uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(uptime_).count();
        double requests_per_second = static_cast<double>(total_requests_) / uptime_seconds;
        double bytes_per_second = static_cast<double>(total_bytes_transferred_) / uptime_seconds;
        
        oss << "Requests per Second: " << std::fixed << std::setprecision(2) << requests_per_second << std::endl;
        oss << "Transfer Rate: " << getFormattedBytes(static_cast<size_t>(bytes_per_second)) << "/s" << std::endl;
    }
    
    return oss.str();
}

std::map<std::string, size_t> FTPStatistics::getStatsMap() const {
    std::map<std::string, size_t> stats;
    
    stats["total_connections"] = total_connections_;
    stats["current_connections"] = current_connections_;
    stats["total_requests"] = total_requests_;
    stats["total_bytes_transferred"] = total_bytes_transferred_;
    stats["total_files_transferred"] = total_files_transferred_;
    stats["successful_logins"] = successful_logins_;
    stats["failed_logins"] = failed_logins_;
    stats["total_errors"] = total_errors_;
    
    return stats;
}

void FTPStatistics::reset() {
    start_time_ = std::chrono::steady_clock::now();
    total_connections_ = 0;
    current_connections_ = 0;
    total_requests_ = 0;
    total_bytes_transferred_ = 0;
    total_files_transferred_ = 0;
    successful_logins_ = 0;
    failed_logins_ = 0;
    total_errors_ = 0;
}

} // namespace ssftpd
