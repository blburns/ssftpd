# Makefile for ssftpd
# Simple-Secure FTP Daemon - A secure, configurable, and feature-rich FTP server
# Copyright 2024 SimpleDaemons
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#     http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Variables
PROJECT_NAME = ssftpd
VERSION = 0.1.0
BUILD_DIR = build
DIST_DIR = dist
PACKAGE_DIR = packaging

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(OS),Windows_NT)
    PLATFORM = windows
    # Windows specific settings
    CXX = cl
    CXXFLAGS = /std:c++17 /W3 /O2 /DNDEBUG /EHsc
    LDFLAGS = ws2_32.lib crypt32.lib
    # Windows specific flags
    CXXFLAGS += /DWIN32 /D_WINDOWS /D_CRT_SECURE_NO_WARNINGS
    # Detect processor cores for parallel builds
    PARALLEL_JOBS = $(shell echo %NUMBER_OF_PROCESSORS%)
    # Windows install paths
    INSTALL_PREFIX = C:/Program Files/$(PROJECT_NAME)
    CONFIG_DIR = $(INSTALL_PREFIX)/etc
    # Windows file extensions
    EXE_EXT = .exe
    LIB_EXT = .lib
    DLL_EXT = .dll
    # Windows commands
    RM = del /Q
    RMDIR = rmdir /S /Q
    MKDIR = mkdir
    CP = copy
    # Check if running in Git Bash or similar
    ifeq ($(shell echo $$SHELL),/usr/bin/bash)
        # Running in Git Bash, use Unix commands
        RM = rm -rf
        RMDIR = rm -rf
        MKDIR = mkdir -p
        CP = cp -r
        # Use Unix-style paths
        INSTALL_PREFIX = /c/Program\ Files/$(PROJECT_NAME)
        CONFIG_DIR = /c/Program\ Files/$(PROJECT_NAME)/etc
    endif
else ifeq ($(UNAME_S),Darwin)
    PLATFORM = macos
    CXX = clang++
    CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2 -DNDEBUG
    LDFLAGS = -lssl -lcrypto
    # macOS specific flags
    CXXFLAGS += -target x86_64-apple-macos12.0 -target arm64-apple-macos12.0
    # Detect processor cores for parallel builds
    PARALLEL_JOBS = $(shell sysctl -n hw.ncpu)
    # macOS install paths
    INSTALL_PREFIX = /usr/local
    CONFIG_DIR = $(INSTALL_PREFIX)/etc/$(PROJECT_NAME)
    # Unix file extensions
    EXE_EXT =
    LIB_EXT = .dylib
    DLL_EXT = .dylib
    # Unix commands
    RM = rm -rf
    RMDIR = rm -rf
    MKDIR = mkdir -p
    CP = cp -r
else
    PLATFORM = linux
    CXX = g++
    CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2 -DNDEBUG
    LDFLAGS = -lssl -lcrypto -lpthread
    # Linux specific flags
    PARALLEL_JOBS = $(shell nproc)
    # Linux install paths
    INSTALL_PREFIX = /usr/local
    CONFIG_DIR = /etc/$(PROJECT_NAME)
    # Unix file extensions
    EXE_EXT =
    LIB_EXT = .so
    DLL_EXT = .so
    # Unix commands
    RM = rm -rf
    RMDIR = rm -rf
    MKDIR = mkdir -p
    CP = cp -r
endif

# Directories
SRC_DIR = src
INCLUDE_DIR = include
CONFIG_DIR_SRC = config
SCRIPTS_DIR = scripts
DEPLOYMENT_DIR = deployment

# Legacy variables for compatibility
INSTALL_DIR = $(INSTALL_PREFIX)
LOG_DIR = /var/log
DATA_DIR = /var/lib/$(PROJECT_NAME)

# Default target
all: build

# Create build directory
$(BUILD_DIR)-dir:
ifeq ($(PLATFORM),windows)
	$(MKDIR) $(BUILD_DIR)
else
	$(MKDIR) $(BUILD_DIR)
endif

# Build using CMake
build: $(BUILD_DIR)-dir
ifeq ($(PLATFORM),windows)
	cd $(BUILD_DIR) && cmake .. -G "Visual Studio 16 2019" -A x64 && cmake --build . --config Release
else
	cd $(BUILD_DIR) && cmake .. && make -j$(PARALLEL_JOBS)
endif

# Clean build
clean:
ifeq ($(PLATFORM),windows)
	$(RMDIR) $(BUILD_DIR)
	$(RMDIR) $(DIST_DIR)
else
	$(RM) $(BUILD_DIR)
	$(RM) $(DIST_DIR)
endif

# Install
install: build
ifeq ($(PLATFORM),windows)
	cd $(BUILD_DIR) && cmake --install . --prefix "$(INSTALL_PREFIX)"
else
	cd $(BUILD_DIR) && sudo make install
endif

# Uninstall
uninstall:
ifeq ($(PLATFORM),windows)
	$(RMDIR) "$(INSTALL_PREFIX)"
else
	sudo rm -f $(INSTALL_PREFIX)/bin/$(PROJECT_NAME)
	sudo rm -f $(INSTALL_PREFIX)/lib/lib$(PROJECT_NAME).so
	sudo rm -f $(INSTALL_PREFIX)/lib/lib$(PROJECT_NAME).dylib
	sudo rm -rf $(INSTALL_PREFIX)/include/$(PROJECT_NAME)
	sudo rm -rf $(CONFIG_DIR)
endif

# Test
test: build
ifeq ($(PLATFORM),windows)
	cd $(BUILD_DIR) && ctest --output-on-failure
else
	cd $(BUILD_DIR) && make test
endif

# Generic package target (platform-specific)
package: build
ifeq ($(PLATFORM),macos)
	@echo "Building macOS packages..."
	@mkdir -p $(DIST_DIR)
	cd $(BUILD_DIR) && cpack -G DragNDrop
	cd $(BUILD_DIR) && cpack -G productbuild
	mv $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.dmg $(DIST_DIR)/ 2>/dev/null || true
	mv $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.pkg $(DIST_DIR)/ 2>/dev/null || true
	@echo "macOS packages created: DMG and PKG"
else ifeq ($(PLATFORM),linux)
	@echo "Building Linux packages..."
	@mkdir -p $(DIST_DIR)
	cd $(BUILD_DIR) && cpack -G RPM
	cd $(BUILD_DIR) && cpack -G DEB
	mv $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.rpm $(DIST_DIR)/ 2>/dev/null || true
	mv $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.deb $(DIST_DIR)/ 2>/dev/null || true
	@echo "Linux packages created: RPM and DEB"
else ifeq ($(PLATFORM),windows)
	@echo "Building Windows packages..."
	@$(MKDIR) $(DIST_DIR)
	cd $(BUILD_DIR) && cpack -G WIX
	cd $(BUILD_DIR) && cpack -G ZIP
	$(CP) $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.msi $(DIST_DIR)/ 2>/dev/null || true
	$(CP) $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.zip $(DIST_DIR)/ 2>/dev/null || true
	@echo "Windows packages created: MSI and ZIP"
else
	@echo "Package generation not supported on this platform"
endif

# Help - Main help (most common targets)
help:
	@echo "Simple Secure FTP Daemon - Main Help"
	@echo "===================================="
	@echo ""
	@echo "Essential targets:"
	@echo "  all              - Build the project (default)"
	@echo "  build            - Build using CMake"
	@echo "  clean            - Clean build files"
	@echo "  install          - Install the project"
	@echo "  uninstall        - Uninstall the project"
	@echo "  test             - Run tests"
	@echo "  package          - Build platform-specific packages"
	@echo "  package-source   - Create source code package"
	@echo ""
	@echo "Development targets:"
	@echo "  dev-build        - Build in debug mode"
	@echo "  dev-test         - Run tests in debug mode"
	@echo "  format           - Format source code"
	@echo "  lint             - Run static analysis"
	@echo "  security-scan    - Run security scanning tools"
	@echo ""
	@echo "Dependency management:"
	@echo "  deps             - Install dependencies"
	@echo "  dev-deps         - Install development tools"
	@echo ""
	@echo "Service management:"
	@echo "  service-install  - Install system service"
	@echo "  service-status   - Check service status"
	@echo "  service-start    - Start service"
	@echo "  service-stop     - Stop service"
	@echo ""
	@echo "Configuration setup:"
	@echo "  setup-ssl        - Setup SSL certificates"
	@echo "  setup-users      - Setup user accounts"
	@echo "  setup-virtuals   - Setup virtual hosts"
	@echo "  setup            - Complete setup"
	@echo ""
	@echo "Help categories:"
	@echo "  help-all         - Show all available targets"
	@echo "  help-build       - Build and development targets"
	@echo "  help-package     - Package creation targets"
	@echo "  help-deps        - Dependency management targets"
	@echo "  help-service     - Service management targets"
	@echo "  help-docker      - Docker targets"
	@echo "  help-config      - Configuration management targets"
	@echo "  help-platform    - Platform-specific targets"
	@echo ""
	@echo "Examples:"
	@echo "  make build       - Build the project"
	@echo "  make test        - Build and run tests"
	@echo "  make package     - Create platform-specific packages"
	@echo "  make dev-deps    - Install development tools"
	@echo "  make help-all    - Show all available targets"

# Development targets
dev-build: $(BUILD_DIR)-dir
ifeq ($(PLATFORM),windows)
	cd $(BUILD_DIR) && cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Debug && cmake --build . --config Debug
else
	cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j$(PARALLEL_JOBS)
endif

dev-test: dev-build
ifeq ($(PLATFORM),windows)
	cd $(BUILD_DIR) && ctest --output-on-failure -C Debug
else
	cd $(BUILD_DIR) && make test
endif

# Code formatting
format:
	@echo "Formatting code..."
	@find $(SRC_DIR) $(INCLUDE_DIR) -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Static analysis
lint: build
	@echo "Running static analysis..."
	cd $(BUILD_DIR) && make clean && scan-build make

# Security scanning
security-scan: build
	@echo "Running security scan..."
	@find $(SRC_DIR) $(INCLUDE_DIR) -name "*.cpp" -o -name "*.hpp" | xargs bandit -r .

# Dependency management
deps:
ifeq ($(PLATFORM),macos)
	@echo "Installing dependencies for macOS..."
	brew install openssl jsoncpp
else ifeq ($(PLATFORM),linux)
	@echo "Installing dependencies for Linux..."
	sudo apt-get update
	sudo apt-get install -y libssl-dev libjsoncpp-dev
else
	@echo "Dependency installation not configured for this platform"
endif

dev-deps:
ifeq ($(PLATFORM),macos)
	@echo "Installing development dependencies for macOS..."
	brew install cmake openssl jsoncpp clang-format cppcheck
else ifeq ($(PLATFORM),linux)
	@echo "Installing development dependencies for Linux..."
	sudo apt-get update
	sudo apt-get install -y build-essential cmake libssl-dev libjsoncpp-dev clang-format cppcheck
else
	@echo "Development dependencies installation not configured for this platform"
endif

# Service management
service-install:
ifeq ($(PLATFORM),macos)
	@echo "Installing macOS service..."
	sudo cp etc/launchd/com.ssftpd.ssftpd.plist /Library/LaunchDaemons/
	sudo chown root:wheel /Library/LaunchDaemons/com.ssftpd.ssftpd.plist
	sudo chmod 644 /Library/LaunchDaemons/com.ssftpd.ssftpd.plist
	sudo launchctl load /Library/LaunchDaemons/com.ssftpd.ssftpd.plist
else ifeq ($(PLATFORM),linux)
	@echo "Installing Linux service..."
	sudo cp etc/systemd/ssftpd.service /etc/systemd/system/
	sudo systemctl daemon-reload
	sudo systemctl enable ssftpd
else
	@echo "Service installation not configured for this platform"
endif

service-status:
ifeq ($(PLATFORM),macos)
	@echo "Checking macOS service status..."
	sudo launchctl list | grep ssftpd
else ifeq ($(PLATFORM),linux)
	@echo "Checking Linux service status..."
	sudo systemctl status ssftpd
else
	@echo "Service status check not configured for this platform"
endif

service-start:
ifeq ($(PLATFORM),macos)
	@echo "Starting macOS service..."
	sudo launchctl start com.ssftpd.ssftpd
else ifeq ($(PLATFORM),linux)
	@echo "Starting Linux service..."
	sudo systemctl start ssftpd
else
	@echo "Service start not configured for this platform"
endif

service-stop:
ifeq ($(PLATFORM),macos)
	@echo "Stopping macOS service..."
	sudo launchctl stop com.ssftpd.ssftpd
else ifeq ($(PLATFORM),linux)
	@echo "Stopping Linux service..."
	sudo systemctl stop ssftpd
else
	@echo "Service stop not configured for this platform"
endif

# Configuration management
install-config:
	@echo "Installing configuration files..."
	sudo mkdir -p $(CONFIG_DIR)
	sudo cp -r $(CONFIG_DIR_SRC)/* $(CONFIG_DIR)/
	sudo chown -R root:root $(CONFIG_DIR)
	sudo chmod -R 644 $(CONFIG_DIR)/*
	sudo chmod 755 $(CONFIG_DIR)

# SSL certificate management
setup-ssl:
	@echo "Setting up SSL certificates..."
	@mkdir -p $(CONFIG_DIR)/ssl
	@echo "Please place your SSL certificates in $(CONFIG_DIR)/ssl/"
	@echo "Required files:"
	@echo "  - server.crt (server certificate)"
	@echo "  - server.key (private key)"
	@echo "  - ca.crt (CA certificate, optional)"
	@echo ""
	@echo "To generate self-signed certificates:"
	@echo "  openssl req -x509 -newkey rsa:4096 -keyout $(CONFIG_DIR)/ssl/server.key -out $(CONFIG_DIR)/ssl/server.crt -days 365 -nodes"

# User management
setup-users:
	@echo "Setting up user accounts..."
	@mkdir -p $(CONFIG_DIR)/users
	@echo "Please configure user accounts in $(CONFIG_DIR)/users/"
	@echo "Example user configuration:"
	@echo "  [user:testuser]"
	@echo "  password = hashed_password"
	@echo "  home_dir = /var/ftp/testuser"
	@echo "  permissions = read,write,list"

# Virtual host setup
setup-virtuals:
	@echo "Setting up virtual hosts..."
	@mkdir -p $(CONFIG_DIR)/virtuals
	@echo "Please configure virtual hosts in $(CONFIG_DIR)/virtuals/"
	@echo "Example virtual host configuration:"
	@echo "  [virtual:example.com]"
	@echo "  bind_address = 0.0.0.0"
	@echo "  bind_port = 21"
	@echo "  ssl_enabled = true"
	@echo "  ssl_cert = /etc/ssftpd/ssl/example.com.crt"
	@echo "  ssl_key = /etc/ssftpd/ssl/example.com.key"

# Complete setup
setup: install install-config setup-ssl setup-users setup-virtuals service-install
	@echo "Complete setup finished!"
	@echo "Configuration directory: $(CONFIG_DIR)"
	@echo "Log directory: /var/log/$(PROJECT_NAME)"
	@echo "SSL directory: $(CONFIG_DIR)/ssl"
	@echo "Users directory: $(CONFIG_DIR)/users"
	@echo "Virtual hosts directory: $(CONFIG_DIR)/virtuals"

# Clean setup
clean-setup: service-stop uninstall
	@echo "Clean setup finished!"

# Package source code
package-source:
	@echo "Creating source package..."
	@mkdir -p $(DIST_DIR)
	@tar -czf $(DIST_DIR)/$(PROJECT_NAME)-$(VERSION)-source.tar.gz \
		--exclude=$(BUILD_DIR) \
		--exclude=$(DIST_DIR) \
		--exclude=.git \
		--exclude="*.o" \
		--exclude="*.so" \
		--exclude="*.dylib" \
		--exclude="*.exe" \
		.

# Docker targets
docker-build:
	@echo "Building Docker image..."
	docker-compose build

docker-run:
	@echo "Running Docker container..."
	docker-compose up -d

docker-stop:
	@echo "Stopping Docker container..."
	docker-compose down

docker-logs:
	@echo "Showing Docker logs..."
	docker-compose logs -f

# Help targets
help-all: help help-build help-package help-deps help-service help-docker help-config help-platform

help-build:
	@echo "Build and Development Targets:"
	@echo "  build            - Build using CMake"
	@echo "  dev-build        - Build in debug mode"
	@echo "  dev-test         - Run tests in debug mode"
	@echo "  clean            - Clean build files"
	@echo "  format           - Format source code"
	@echo "  lint             - Run static analysis"
	@echo "  security-scan    - Run security scanning"

help-package:
	@echo "Package Creation Targets:"
	@echo "  package          - Build platform-specific packages"
	@echo "  package-source   - Create source code package"

help-deps:
	@echo "Dependency Management Targets:"
	@echo "  deps             - Install runtime dependencies"
	@echo "  dev-deps         - Install development dependencies"

help-service:
	@echo "Service Management Targets:"
	@echo "  service-install  - Install system service"
	@echo "  service-status   - Check service status"
	@echo "  service-start    - Start service"
	@echo "  service-stop     - Stop service"

help-docker:
	@echo "Docker Targets:"
	@echo "  docker-build     - Build Docker image"
	@echo "  docker-run       - Run Docker container"
	@echo "  docker-stop      - Stop Docker container"
	@echo "  docker-logs      - Show Docker logs"

help-config:
	@echo "Configuration Management Targets:"
	@echo "  install-config   - Install configuration files"
	@echo "  setup-ssl        - Setup SSL certificates"
	@echo "  setup-users      - Setup user accounts"
	@echo "  setup-virtuals   - Setup virtual hosts"
	@echo "  setup            - Complete setup"
	@echo "  clean-setup      - Clean complete setup"

help-platform:
	@echo "Platform Information:"
	@echo "  Platform: $(PLATFORM)"
	@echo "  Install prefix: $(INSTALL_PREFIX)"
	@echo "  Config directory: $(CONFIG_DIR)"
	@echo "  Parallel jobs: $(PARALLEL_JOBS)"

# Phony targets
.PHONY: all build clean install uninstall test package package-source help \
        dev-build dev-test format lint security-scan deps dev-deps \
        service-install service-status service-start service-stop \
        install-config setup-ssl setup-users setup-virtuals setup clean-setup \
        docker-build docker-run docker-stop docker-logs \
        help-all help-build help-package help-deps help-service help-docker help-config help-platform

# Default target
.DEFAULT_GOAL := all
