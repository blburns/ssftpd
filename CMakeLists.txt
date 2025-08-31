cmake_minimum_required(VERSION 3.16)
# Get system architecture for package naming
execute_process(COMMAND uname -m OUTPUT_VARIABLE SYSTEM_ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
message(STATUS "System architecture: ${SYSTEM_ARCH}")

project(ssftpd-${SYSTEM_ARCH} VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS deployment version" FORCE)
    # Allow command line override of architectures
    if(NOT DEFINED CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Build architectures for macOS" FORCE)
    endif()
    set(PLATFORM_MACOS TRUE)
    message(STATUS "Building for macOS (Monterey 12.0+) with architectures: ${CMAKE_OSX_ARCHITECTURES}")
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_LINUX TRUE)
    message(STATUS "Building for Linux")
elseif(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    message(STATUS "Building for Windows")
    
    # Windows-specific settings
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
    
    # Set Windows SDK version if available
    if(DEFINED ENV{WINDOWS_SDK_VERSION})
        set(CMAKE_SYSTEM_VERSION $ENV{WINDOWS_SDK_VERSION})
    endif()
endif()

# Build options
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" OFF)
option(ENABLE_LOGGING "Enable logging" ON)
option(ENABLE_SSL "Enable SSL/TLS support" ON)
option(USE_SYSTEM_LIBS "Use system libraries instead of Homebrew" OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find required packages
find_package(PkgConfig REQUIRED)

# Find OpenSSL
if(ENABLE_SSL)
    if(USE_SYSTEM_LIBS)
        # Try to find system OpenSSL first
        find_package(OpenSSL QUIET)
        if(NOT OpenSSL_FOUND)
            # Fall back to Homebrew OpenSSL
            find_package(OpenSSL REQUIRED)
        endif()
    else()
        find_package(OpenSSL REQUIRED)
    endif()
endif()

# Find jsoncpp for JSON parsing
find_package(PkgConfig REQUIRED)
# Temporarily disable jsoncpp to get project building
# pkg_check_modules(JSONCPP jsoncpp)
set(JSONCPP_FOUND FALSE)

# For macOS, try multiple approaches to find jsoncpp
if(PLATFORM_MACOS)
    if(NOT JSONCPP_FOUND)
        # Try to find jsoncpp without pkg-config
        find_library(JSONCPP_LIBRARIES jsoncpp)
        find_path(JSONCPP_INCLUDE_DIRS json/json.h)
        if(JSONCPP_LIBRARIES AND JSONCPP_INCLUDE_DIRS)
            set(JSONCPP_FOUND TRUE)
            set(JSONCPP_LIBRARIES ${JSONCPP_LIBRARIES})
            set(JSONCPP_INCLUDE_DIRS ${JSONCPP_INCLUDE_DIRS})
        endif()
    endif()
    
    # If still not found, try common Homebrew paths
    if(NOT JSONCPP_FOUND)
        find_library(JSONCPP_LIBRARIES 
            NAMES jsoncpp
            PATHS 
            /usr/local/lib
            /usr/local/Cellar/jsoncpp/*/lib
            /opt/homebrew/lib
            /opt/homebrew/Cellar/jsoncpp/*/lib
            NO_DEFAULT_PATH
        )
        find_path(JSONCPP_INCLUDE_DIRS 
            NAMES json/json.h
            PATHS 
            /usr/local/include
            /usr/local/Cellar/jsoncpp/*/include
            /opt/homebrew/include
            /opt/homebrew/Cellar/jsoncpp/*/include
            NO_DEFAULT_PATH
        )
        if(JSONCPP_LIBRARIES AND JSONCPP_INCLUDE_DIRS)
            set(JSONCPP_FOUND TRUE)
            message(STATUS "Found jsoncpp via direct path search")
        endif()
    endif()
    
    # Ensure we have the full library path
    if(JSONCPP_FOUND AND JSONCPP_LIBRARIES)
        get_filename_component(JSONCPP_LIBRARY_DIR ${JSONCPP_LIBRARIES} DIRECTORY)
        if(JSONCPP_LIBRARY_DIR)
            link_directories(${JSONCPP_LIBRARY_DIR})
            message(STATUS "Added jsoncpp library directory: ${JSONCPP_LIBRARY_DIR}")
        endif()
    endif()
else()
    # Non-macOS systems
    if(NOT JSONCPP_FOUND)
        find_library(JSONCPP_LIBRARIES jsoncpp)
        find_path(JSONCPP_INCLUDE_DIRS json/json.h)
        if(JSONCPP_LIBRARIES AND JSONCPP_INCLUDE_DIRS)
            set(JSONCPP_FOUND TRUE)
            set(JSONCPP_LIBRARIES ${JSONCPP_LIBRARIES})
            set(JSONCPP_INCLUDE_DIRS ${JSONCPP_INCLUDE_DIRS})
        endif()
    endif()
endif()

if(NOT JSONCPP_FOUND)
    message(WARNING "jsoncpp not found. JSON configuration parsing will not be available.")
    add_definitions(-DDISABLE_JSON_CONFIG)
    # Set empty variables to avoid linking errors
    set(JSONCPP_LIBRARIES "")
    set(JSONCPP_INCLUDE_DIRS "")
else()
    message(STATUS "Found jsoncpp: ${JSONCPP_LIBRARIES}")
    add_definitions(-DENABLE_JSON_CONFIG)
endif()

# Platform-specific library handling
if(PLATFORM_MACOS)
    # macOS specific libraries
    find_library(SECURITY_FRAMEWORK Security)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
    find_library(SYSTEM_CONFIGURATION_FRAMEWORK SystemConfiguration)
    
    # macOS doesn't need separate socket or rt libraries
    set(PLATFORM_LIBRARIES 
        ${SECURITY_FRAMEWORK}
        ${CORE_FOUNDATION_FRAMEWORK}
        ${SYSTEM_CONFIGURATION_FRAMEWORK}
    )
    
    # Set macOS-specific compiler flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    
    # Enable universal binary support for Intel + Apple Silicon
    if(CMAKE_OSX_ARCHITECTURES MATCHES ".*arm64.*")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -target arm64-apple-macos12.0")
    endif()
    if(CMAKE_OSX_ARCHITECTURES MATCHES ".*x86_64.*")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -target x86_64-apple-macos12.0")
    endif()
    
elseif(PLATFORM_LINUX)
    # Linux specific libraries
    # Socket functions are part of libc, no need to find_library
    # RT library for real-time functions
    find_library(RT_LIBRARY rt)
    
    set(PLATFORM_LIBRARIES 
        ${RT_LIBRARY}
        pthread
    )
    
    # Linux compiler flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    
elseif(PLATFORM_WINDOWS)
    # Windows specific libraries
    set(PLATFORM_LIBRARIES 
        ws2_32
        iphlpapi
        crypt32
        advapi32
        userenv
        shell32
    )
    
    # Windows compiler flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
    
    # Windows-specific definitions
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DNOMINMAX)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# Add jsoncpp include directories if found
if(JSONCPP_FOUND)
    include_directories(${JSONCPP_INCLUDE_DIRS})
endif()

# Source files (exclude tests)
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/core/*.cpp"
    "src/utils/*.cpp"
)

# Filter out test files from main sources
list(FILTER SOURCES EXCLUDE REGEX ".*test.*\\.cpp$")

# Header files
file(GLOB_RECURSE HEADERS 
    "include/*.hpp"
    "src/*.hpp"
)

# Create main executable
add_executable(ssftpd-${SYSTEM_ARCH} ${SOURCES})

# Create shared library
add_library(ssftpd-lib-${SYSTEM_ARCH} SHARED ${SOURCES})
set_target_properties(ssftpd-lib-${SYSTEM_ARCH} PROPERTIES
    OUTPUT_NAME "ssftpd"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Platform-specific target properties
if(PLATFORM_MACOS)
    set_target_properties(ssftpd-${SYSTEM_ARCH} PROPERTIES
        MACOSX_BUNDLE FALSE
        MACOSX_RPATH "@executable_path/../lib"
    )
    set_target_properties(ssftpd-lib-${SYSTEM_ARCH} PROPERTIES
        MACOSX_RPATH "@loader_path"
    )
elseif(PLATFORM_WINDOWS)
    set_target_properties(ssftpd-${SYSTEM_ARCH} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    set_target_properties(ssftpd-lib-${SYSTEM_ARCH} PROPERTIES
        PREFIX ""
        SUFFIX ".dll"
    )
endif()

# Link libraries
target_link_libraries(ssftpd-${SYSTEM_ARCH}
    ssftpd-lib-${SYSTEM_ARCH}
    ${OPENSSL_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    ${PLATFORM_LIBRARIES}
)

# Add linker flags to suppress dylib warnings on macOS
if(PLATFORM_MACOS)
    set_target_properties(ssftpd-${SYSTEM_ARCH} PROPERTIES
        LINK_FLAGS "-Wl,-w"
    )
endif()

target_link_libraries(ssftpd-lib-${SYSTEM_ARCH}
    ${OPENSSL_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    ${PLATFORM_LIBRARIES}
)

# Add linker flags to suppress dylib warnings on macOS
if(PLATFORM_MACOS)
    set_target_properties(ssftpd-lib-${SYSTEM_ARCH} PROPERTIES
        LINK_FLAGS "-Wl,-w"
    )
endif()

# Compiler flags
target_compile_options(ssftpd-${SYSTEM_ARCH} PRIVATE
    -Wall -Wextra -Wpedantic
    -O2 -DNDEBUG
)

target_compile_options(ssftpd-lib-${SYSTEM_ARCH} PRIVATE
    -Wall -Wextra -Wpedantic
    -O2 -DNDEBUG
    -fPIC
)

# Platform-specific compile definitions
if(PLATFORM_MACOS)
    target_compile_definitions(ssftpd-${SYSTEM_ARCH} PRIVATE PLATFORM_MACOS=1)
    target_compile_definitions(ssftpd-lib-${SYSTEM_ARCH} PRIVATE PLATFORM_MACOS=1)
elseif(PLATFORM_LINUX)
    target_compile_definitions(ssftpd-${SYSTEM_ARCH} PRIVATE PLATFORM_LINUX=1)
    target_compile_definitions(ssftpd-lib-${SYSTEM_ARCH} PRIVATE PLATFORM_LINUX=1)
elseif(PLATFORM_WINDOWS)
    target_compile_definitions(ssftpd-${SYSTEM_ARCH} PRIVATE PLATFORM_WINDOWS=1)
    target_compile_definitions(ssftpd-lib-${SYSTEM_ARCH} PRIVATE PLATFORM_WINDOWS=1)
endif()

# Install targets with platform-specific destinations
if(PLATFORM_MACOS)
    # macOS install paths
    install(TARGETS ssftpd-${SYSTEM_ARCH} ssftpd-lib-${SYSTEM_ARCH}
        RUNTIME DESTINATION /usr/local/bin
        LIBRARY DESTINATION /usr/local/lib
        ARCHIVE DESTINATION /usr/local/lib
    )
    install(DIRECTORY include/ DESTINATION /usr/local/include)
    install(DIRECTORY config/ DESTINATION /usr/local/etc/ssftpd)
    install(DIRECTORY tools/ DESTINATION /usr/local/share/ssftpd/tools)
    install(DIRECTORY docs/ DESTINATION /usr/local/share/ssftpd/docs)
    install(FILES LICENSE README.md DESTINATION /usr/local/share/ssftpd)
    
    # Create additional directories for macOS
    install(CODE "
        # Create additional directories for logs and SSL
        execute_process(COMMAND \${CMAKE_COMMAND} -E make_directory /usr/local/var/log/ssftpd
            WORKING_DIRECTORY \${CMAKE_INSTALL_PREFIX}
            ERROR_QUIET
        )
        execute_process(COMMAND \${CMAKE_COMMAND} -E make_directory /usr/local/etc/ssftpd/ssl
            WORKING_DIRECTORY \${CMAKE_INSTALL_PREFIX}
            ERROR_QUIET
        )
    ")
    
elseif(PLATFORM_WINDOWS)
    # Windows install paths
    install(TARGETS ssftpd-${SYSTEM_ARCH} ssftpd-lib-${SYSTEM_ARCH}
        RUNTIME DESTINATION "C:/Program Files/ssftpd/bin"
        LIBRARY DESTINATION "C:/Program Files/ssftpd/lib"
        ARCHIVE DESTINATION "C:/Program Files/ssftpd/lib"
    )
    install(DIRECTORY include/ DESTINATION "C:/Program Files/ssftpd/include")
    install(DIRECTORY config/ DESTINATION "C:/Program Files/ssftpd/config")
    install(DIRECTORY tools/ DESTINATION "C:/Program Files/ssftpd/tools")
    install(DIRECTORY docs/ DESTINATION "C:/Program Files/ssftpd/docs")
    install(FILES LICENSE README.md DESTINATION "C:/Program Files/ssftpd")
    
    # Create additional directories for Windows
    install(CODE "
        # Create additional directories for logs and SSL
        execute_process(COMMAND \${CMAKE_COMMAND} -E make_directory \"C:/Program Files/ssftpd/logs\"
            WORKING_DIRECTORY \${CMAKE_INSTALL_PREFIX}
            ERROR_QUIET
        )
        execute_process(COMMAND \${CMAKE_COMMAND} -E make_directory \"C:/Program Files/ssftpd/ssl\"
            WORKING_DIRECTORY \${CMAKE_INSTALL_PREFIX}
            ERROR_QUIET
        )
        execute_process(COMMAND \${CMAKE_COMMAND} -E make_directory \"C:/Program Files/ssftpd/temp\"
            WORKING_DIRECTORY \${CMAKE_INSTALL_PREFIX}
            ERROR_QUIET
        )
    ")
    
else()
    # Linux install paths - Follow FHS standards
    install(TARGETS ssftpd-${SYSTEM_ARCH} ssftpd-lib-${SYSTEM_ARCH}
        RUNTIME DESTINATION usr/bin
        LIBRARY DESTINATION usr/lib
        ARCHIVE DESTINATION usr/lib
    )
    install(DIRECTORY include/ DESTINATION usr/include)
    install(DIRECTORY config/ DESTINATION etc/ssftpd)
    install(DIRECTORY tools/ DESTINATION usr/share/ssftpd/tools)
    install(DIRECTORY docs/ DESTINATION usr/share/ssftpd/docs)
    install(FILES LICENSE README.md DESTINATION usr/share/ssftpd)
    
    # Create symlinks for convenience (similar to alternatives)
    install(CODE "
        # Create symlink for ssftpd binary
        execute_process(COMMAND \${CMAKE_COMMAND} -E create_symlink 
            /usr/bin/ssftpd-${SYSTEM_ARCH} /usr/local/bin/ssftpd
            WORKING_DIRECTORY \${CMAKE_INSTALL_PREFIX}
            ERROR_QUIET
        )
        # Create symlink for ssftpd-lib
        execute_process(COMMAND \${CMAKE_COMMAND} -E create_symlink 
            /usr/lib/libssftpd-lib-${SYSTEM_ARCH}.so /usr/local/lib/libssftpd-lib.so
            WORKING_DIRECTORY \${CMAKE_INSTALL_PREFIX}
            ERROR_QUIET
        )
        
        # Create additional directories for logs and SSL
        execute_process(COMMAND \${CMAKE_COMMAND} -E make_directory /var/log/ssftpd
            WORKING_DIRECTORY \${CMAKE_INSTALL_PREFIX}
            ERROR_QUIET
        )
        execute_process(COMMAND \${CMAKE_COMMAND} -E make_directory /etc/ssftpd/ssl
            WORKING_DIRECTORY \${CMAKE_INSTALL_PREFIX}
            ERROR_QUIET
        )
    ")
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(src/tests)
endif()

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ssftpd-${SYSTEM_ARCH}-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ssftpd-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ssftpd-${SYSTEM_ARCH}-config.cmake"
    @ONLY
)

# Install CMake package configuration files
if(PLATFORM_MACOS)
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/ssftpd-${SYSTEM_ARCH}-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/ssftpd-${SYSTEM_ARCH}-config-version.cmake"
        DESTINATION /usr/local/lib/cmake/ssftpd-${SYSTEM_ARCH}
    )
elseif(PLATFORM_WINDOWS)
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/ssftpd-${SYSTEM_ARCH}-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/ssftpd-${SYSTEM_ARCH}-config-version.cmake"
        DESTINATION "C:/Program Files/ssftpd/lib/cmake/ssftpd-${SYSTEM_ARCH}"
    )
else()
    # Linux - standard FHS location
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/ssftpd-${SYSTEM_ARCH}-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/ssftpd-${SYSTEM_ARCH}-config-version.cmake"
        DESTINATION usr/lib/cmake/ssftpd-${SYSTEM_ARCH}
    )
endif()

# macOS package generation
if(PLATFORM_MACOS AND BUILD_SHARED_LIBS)
    # Create macOS packages (DMG and PKG)
    set(CPACK_GENERATOR "DragNDrop;productbuild")
    set(CPACK_PACKAGE_NAME "ssftpd")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Simple FTP Daemon for macOS")
    set(CPACK_PACKAGE_VENDOR "ssftpd")
    set(CPACK_PACKAGE_CONTACT "SimpleDaemons")
    
    # Set the correct package naming format: {VERSION}-{OS}-{ARCH}
    set(CPACK_PACKAGE_FILE_NAME "ssftpd-${PROJECT_VERSION}-macOS-${SYSTEM_ARCH}")
    
    # DMG specific settings
    set(CPACK_DMG_VOLUME_NAME "ssftpd-${PROJECT_VERSION}-macOS-${SYSTEM_ARCH}")
    set(CPACK_DMG_FORMAT "UDZO")
    
    # PKG specific settings (productbuild)
    set(CPACK_PRODUCTBUILD_PACKAGE_NAME "ssftpd-${PROJECT_VERSION}-macOS-${SYSTEM_ARCH}")
    set(CPACK_PRODUCTBUILD_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PRODUCTBUILD_PACKAGE_DESCRIPTION "Simple FTP Daemon for macOS - ${SYSTEM_ARCH}")
    set(CPACK_PRODUCTBUILD_PACKAGE_VENDOR "ssftpd")
    set(CPACK_PRODUCTBUILD_PACKAGE_CONTACT "SimpleDaemons")
    
    include(CPack)
    
# Linux package generation
elseif(PLATFORM_LINUX AND BUILD_SHARED_LIBS)
    # Create Linux packages (DEB, RPM, TGZ)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    set(CPACK_PACKAGE_NAME "ssftpd")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Simple FTP Daemon for Linux")
    set(CPACK_PACKAGE_VENDOR "ssftpd")
    set(CPACK_PACKAGE_CONTACT "SimpleDaemons")
    
    # Set the correct package naming format: {VERSION}-{OS}-{ARCH}
    set(CPACK_PACKAGE_FILE_NAME "ssftpd-${PROJECT_VERSION}-Linux-${SYSTEM_ARCH}")
    
    # DEB specific settings
    set(CPACK_DEBIAN_PACKAGE_NAME "ssftpd-${SYSTEM_ARCH}")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libssl3, libjsoncpp25")
    set(CPACK_DEBIAN_PACKAGE_SECTION "net")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/ssftpd/ssftpd")
    
    # RPM specific settings
    set(CPACK_RPM_PACKAGE_NAME "ssftpd-${SYSTEM_ARCH}")
    set(CPACK_RPM_PACKAGE_REQUIRES "openssl, jsoncpp")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Internet")
    set(CPACK_RPM_PACKAGE_URL "https://github.com/ssftpd/ssftpd")
    
    # TGZ specific settings (source package - no OS/ARCH needed)
    set(CPACK_TGZ_PACKAGE_NAME "ssftpd-${PROJECT_VERSION}-source")
    
    include(CPack)

# Windows package generation
elseif(PLATFORM_WINDOWS AND BUILD_SHARED_LIBS)
    # Create Windows installer
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_NAME "ssftpd")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Simple FTP Daemon for Windows")
    set(CPACK_PACKAGE_VENDOR "ssftpd")
    set(CPACK_PACKAGE_CONTACT "SimpleDaemons")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "ssftpd")
    
    # Set the correct package naming format: {VERSION}-{OS}-{ARCH}
    set(CPACK_PACKAGE_FILE_NAME "ssftpd-${PROJECT_VERSION}-Windows-${SYSTEM_ARCH}")
    
    # NSIS specific settings
    set(CPACK_NSIS_DISPLAY_NAME "ssftpd ${PROJECT_VERSION} (${SYSTEM_ARCH})")
    set(CPACK_NSIS_PACKAGE_NAME "ssftpd-${PROJECT_VERSION}-Windows-${SYSTEM_ARCH}")
    set(CPACK_NSIS_CONTACT "SimpleDaemons")
    set(CPACK_NSIS_URL "https://github.com/ssftpd/ssftpd")
    set(CPACK_NSIS_ENABLE_DIRECTORY_PAGE ON)
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_MENU_LINKS "https://github.com/ssftpd/ssftpd" "Documentation")
    
    # Windows specific settings - follow Windows conventions
    set(CPACK_NSIS_INSTALL_ROOT "C:\\Program Files")
    set(CPACK_NSIS_PROGRAM_MENU_DIR "ssftpd")
    set(CPACK_NSIS_DESKTOP_SHORTCUT ON)
    set(CPACK_NSIS_START_MENU_SHORTCUT ON)
    
    include(CPack)
endif()
