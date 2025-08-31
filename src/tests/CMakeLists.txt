# Tests CMakeLists.txt for simple-sftpd
# Copyright 2024 SimpleDaemons
# Licensed under Apache License 2.0

# Enable testing
enable_testing()

# Find Google Test if available
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    # Try to find GTest without find_package
    find_library(GTEST_LIBRARIES gtest gtest_main)
    find_path(GTEST_INCLUDE_DIRS gtest/gtest.h)
    if(GTEST_LIBRARIES AND GTEST_INCLUDE_DIRS)
        set(GTest_FOUND TRUE)
        set(GTEST_LIBRARIES ${GTEST_LIBRARIES})
        set(GTEST_INCLUDE_DIRS ${GTEST_INCLUDE_DIRS})
    endif()
endif()

if(GTest_FOUND)
    message(STATUS "Found Google Test: ${GTEST_LIBRARIES}")
    
    # Test source files
    set(TEST_SOURCES
        test_main.cpp
        utils/test_helpers.cpp
    )
    
    # Test executable
    add_executable(simple-tftpd-tests ${TEST_SOURCES})
    
    # Link against the main library
    target_link_libraries(simple-tftpd-tests 
        ssftpd-lib-x86_64
        ${GTEST_LIBRARIES}
        ${PLATFORM_LIBRARIES}
    )
    
    # Add the source files that contain the classes being tested
    # Note: These source files don't exist yet - they need to be implemented
    # target_sources(simple-tftpd-tests PRIVATE
    #     ${CMAKE_SOURCE_DIR}/src/utils/logger.cpp
    #     ${CMAKE_SOURCE_DIR}/src/utils/config_parser.cpp
    #     ${CMAKE_SOURCE_DIR}/src/core/ftp_packet.cpp
    #     ${CMAKE_SOURCE_DIR}/src/core/ftp_connection.cpp
    #     ${CMAKE_SOURCE_DIR}/src/core/ftp_server.cpp
    # )
    
    if(JSONCPP_FOUND)
        target_link_libraries(simple-tftpd-tests ${JSONCPP_LIBRARIES})
    endif()
    
    # Include directories
    target_include_directories(simple-tftpd-tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${GTEST_INCLUDE_DIRS}
    )
    
    # Add tests
    add_test(NAME simple-tftpd-tests COMMAND simple-tftpd-tests)
    
    # Set test properties
    set_tests_properties(simple-tftpd-tests PROPERTIES
        TIMEOUT 300
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    
else()
    message(WARNING "Google Test not found. Tests will not be built.")
endif()

# Test configuration
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_config.h.in)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/test_config.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/test_config.h
    )
endif()

# Test data directory
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/data)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()
