#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace simple_tftpd {
namespace test {

/**
 * @brief Test helper utilities for TFTP daemon tests
 * 
 * This class provides various utilities for creating test files,
 * managing test directories, and performing common test operations.
 */
class TestHelpers {
public:
    /**
     * @brief Constructor - creates test environment
     */
    TestHelpers();
    
    /**
     * @brief Destructor - cleans up test environment
     */
    ~TestHelpers();
    
    /**
     * @brief Create a temporary directory for testing
     * @return Path to created directory
     */
    std::string createTempDirectory();
    
    /**
     * @brief Create a test file with specific content
     * @param filename Name of the file to create
     * @param content Content to write to the file
     * @return Path to created file, or empty string on failure
     */
    std::string createTestFile(const std::string& filename, const std::string& content);
    
    /**
     * @brief Create a test file with random data of specified size
     * @param filename Name of the file to create
     * @param size Size of the file in bytes
     * @return Path to created file, or empty string on failure
     */
    std::string createTestFile(const std::string& filename, size_t size);
    
    /**
     * @brief Check if a file exists
     * @param filepath Path to the file to check
     * @return true if file exists, false otherwise
     */
    bool fileExists(const std::string& filepath);
    
    /**
     * @brief Read contents of a file
     * @param filepath Path to the file to read
     * @return File contents as string, or empty string on failure
     */
    std::string readFile(const std::string& filepath);
    
    /**
     * @brief Get the size of a file
     * @param filepath Path to the file
     * @return File size in bytes, or 0 on failure
     */
    size_t getFileSize(const std::string& filepath);
    
    /**
     * @brief Clean up test environment
     */
    void cleanup();
    
    /**
     * @brief Get the test directory path
     * @return Path to test directory
     */
    std::string getTestDirectory() const;
    
    /**
     * @brief Generate a random string of specified length
     * @param length Length of the string to generate
     * @return Random string
     */
    std::string generateRandomString(size_t length);
    
    /**
     * @brief Generate random data of specified size
     * @param size Size of data to generate
     * @return Vector of random bytes
     */
    std::vector<uint8_t> generateRandomData(size_t size);
    
    /**
     * @brief Compare two files for equality
     * @param file1 Path to first file
     * @param file2 Path to second file
     * @return true if files are identical, false otherwise
     */
    bool compareFiles(const std::string& file1, const std::string& file2);
    
    /**
     * @brief Get a valid network interface for testing
     * @return Name of available network interface
     */
    std::string getNetworkInterface();
    
    /**
     * @brief Check if a port is available
     * @param port Port number to check
     * @return true if port is available, false otherwise
     */
    bool isPortAvailable(uint16_t port);
    
    /**
     * @brief Find an available port starting from specified port
     * @param start_port Starting port number
     * @return Available port number, or 0 if none found
     */
    uint16_t findAvailablePort(uint16_t start_port);

private:
    std::string test_dir_; ///< Path to test directory
};

} // namespace test
} // namespace simple_tftpd
