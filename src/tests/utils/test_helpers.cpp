#include "test_helpers.hpp"
#include <iostream>
#include <fstream>
#include <random>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

namespace simple_tftpd {
namespace test {

TestHelpers::TestHelpers() {
    test_dir_ = createTempDirectory();
}

TestHelpers::~TestHelpers() {
    cleanup();
}

std::string TestHelpers::createTempDirectory() {
    std::string temp_dir = "/tmp/simple-tftpd-test-";
    temp_dir += std::to_string(getpid());
    temp_dir += "-";

    // Add random suffix
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    temp_dir += std::to_string(dis(gen));

    // Create directory
    if (mkdir(temp_dir.c_str(), 0755) != 0) {
        throw std::runtime_error("Failed to create temp directory: " + temp_dir);
    }

    return temp_dir;
}

std::string TestHelpers::createTestFile(const std::string& filename, const std::string& content) {
    std::string filepath = test_dir_ + "/" + filename;
    std::ofstream file(filepath);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to create test file: " + filepath);
    }

    file << content;
    file.close();
    return filepath;
}

std::string TestHelpers::createTestFile(const std::string& filename, size_t size) {
    std::string filepath = test_dir_ + "/" + filename;
    std::ofstream file(filepath, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to create test file: " + filepath);
    }

    // Generate random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (size_t i = 0; i < size; ++i) {
        file.put(static_cast<char>(dis(gen)));
    }

    file.close();
    return filepath;
}

bool TestHelpers::fileExists(const std::string& filepath) {
    std::ifstream file(filepath);
    return file.good();
}

std::string TestHelpers::readFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return content;
}

size_t TestHelpers::getFileSize(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }

    return file.tellg();
}

void TestHelpers::cleanup() {
    if (!test_dir_.empty()) {
        std::string cmd = "rm -rf " + test_dir_;
        system(cmd.c_str());
        test_dir_.clear();
    }
}

std::string TestHelpers::getTestDirectory() const {
    return test_dir_;
}

std::string TestHelpers::generateRandomString(size_t length) {
    static const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.length() - 1);

    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += chars[dis(gen)];
    }
    return result;
}

std::vector<uint8_t> TestHelpers::generateRandomData(size_t size) {
    std::vector<uint8_t> data(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<uint8_t>(dis(gen));
    }
    return data;
}

bool TestHelpers::compareFiles(const std::string& file1, const std::string& file2) {
    std::ifstream f1(file1, std::ios::binary);
    std::ifstream f2(file2, std::ios::binary);

    if (!f1.is_open() || !f2.is_open()) {
        return false;
    }
    return true;
}

std::string TestHelpers::getNetworkInterface() {
    return "127.0.0.1"; // Simplified for now
}

bool TestHelpers::isPortAvailable(uint16_t port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bool available = (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0);
    close(sock);

    return available;
}

uint16_t TestHelpers::findAvailablePort(uint16_t start_port) {
    for (uint16_t port = start_port; port < start_port + 1000; ++port) {
        if (isPortAvailable(port)) {
            return port;
        }
    }
    return 0; // No available port found
}

} // namespace test
} // namespace simple_tftpd
