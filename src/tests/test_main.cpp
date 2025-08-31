#include <gtest/gtest.h>
#include "ssftpd/logger.hpp"

// Basic test to ensure the test framework works
TEST(BasicTest, TestFramework) {
    EXPECT_EQ(1, 1);
    EXPECT_TRUE(true);
    EXPECT_FALSE(false);
}

// Test Logger class
TEST(LoggerTest, BasicFunctionality) {
    ssftpd::Logger logger;
    
    // Test default log level
    EXPECT_EQ(logger.getLogLevel(), ssftpd::LogLevel::INFO);
    
    // Test setting log level
    logger.setLogLevel(ssftpd::LogLevel::DEBUG);
    EXPECT_EQ(logger.getLogLevel(), ssftpd::LogLevel::DEBUG);
    
    // Test console logging (can only set, not check state)
    logger.setConsoleOutput(true);
    // Note: No getter method available for console output state
}

// Main function for Google Test
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
