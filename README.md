# Simple-Secure FTP Daemon (ssftpd)

A secure, configurable, and feature-rich FTP server written in C++ for Linux, macOS, and Windows.

## Features

### Core FTP Functionality
- **RFC 959 Compliant**: Full FTP protocol implementation
- **Active/Passive Mode**: Support for both connection modes
- **File Transfer**: Upload, download, append, and resume capabilities
- **Directory Operations**: List, create, remove, and navigate directories
- **File Management**: Rename, delete, and modify file permissions

### Security Features
- **SSL/TLS Support**: Secure file transfers with OpenSSL
- **User Authentication**: Multiple authentication methods (password, hash, PAM, LDAP)
- **Access Control**: Granular permissions and path restrictions
- **Chroot Support**: Isolated file system access
- **Privilege Dropping**: Security hardening for production use
- **Rate Limiting**: Protection against abuse and DoS attacks

### Virtual Hosting
- **Multi-Domain Support**: Host multiple FTP sites on one server
- **Per-Host Configuration**: Individual settings for each virtual host
- **SSL Certificate Management**: Separate certificates per domain
- **Access Control**: User restrictions per virtual host

### User Management
- **Flexible Permissions**: Granular control over user capabilities
- **Anonymous Access**: Configurable anonymous user support
- **Guest Accounts**: Limited access for temporary users
- **Connection Limits**: Per-user connection and transfer restrictions
- **Session Management**: Timeout and activity tracking

### Performance & Monitoring
- **Multi-threaded**: Efficient handling of multiple connections
- **Connection Pooling**: Optimized resource management
- **Transfer Optimization**: Sendfile and memory-mapped I/O support
- **Statistics**: Comprehensive usage and performance metrics
- **Logging**: Advanced logging with rotation and filtering

### Platform Support
- **Cross-Platform**: Linux, macOS, and Windows
- **Native Builds**: Optimized for each platform
- **Package Management**: DEB, RPM, PKG, and MSI packages
- **Service Integration**: systemd, launchd, and Windows services

## Quick Start

### 🐳 Docker (Recommended)

The fastest way to get started with ssftpd is using Docker:

```bash
# Clone the repository
git clone https://github.com/ssftpd/ssftpd.git
cd ssftpd

# Quick start with Docker
cd deployment/examples/docker
docker-compose up -d

# Test the FTP service
nc -z localhost 21
```

**Docker Features:**
- ✅ **Zero dependencies** - No need to install build tools
- ✅ **Cross-platform** - Works on Linux, macOS, Windows
- ✅ **Production-ready** - Optimized runtime image
- ✅ **Development environment** - Full debugging tools included
- ✅ **Multi-architecture** - x86_64, ARM64, ARMv7 support

For detailed Docker deployment, see [Docker Deployment Guide](docs/deployment/docker.md).

### Traditional Installation

#### Prerequisites

- **C++17 Compiler**: GCC 7+, Clang 5+, or MSVC 2017+
- **CMake 3.16+**: Build system
- **OpenSSL**: SSL/TLS support
- **jsoncpp**: JSON configuration parsing

#### From Source

```bash
# Clone the repository
git clone https://github.com/ssftpd/ssftpd.git
cd ssftpd

# Build the project
make install-dev  # Install development dependencies
make build        # Build the application
make install      # Install system-wide
```

#### From Packages

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install ssftpd
```

**CentOS/RHEL:**
```bash
sudo yum install ssftpd
# or
sudo dnf install ssftpd
```

**macOS:**
```bash
brew install ssftpd
```

### Configuration

1. **Copy the example configuration:**
```bash
sudo cp /etc/ssftpd/ssftpd.conf.example /etc/ssftpd/ssftpd.conf
```

2. **Edit the configuration file:**
```bash
sudo nano /etc/ssftpd/ssftpd.conf
```

3. **Create necessary directories:**
```bash
sudo mkdir -p /var/ftp /var/log/ssftpd
sudo chown ftp:ftp /var/ftp
```

### Running the Server

#### Basic Usage

```bash
# Start in foreground (for testing)
sudo ssftpd start --config /etc/ssftpd/ssftpd.conf

# Start as daemon
sudo ssftpd --daemon start

# Test configuration
ssftpd --test-config --config /etc/ssftpd/ssftpd.conf
```

#### Service Management

**Linux (systemd):**
```bash
sudo systemctl enable ssftpd
sudo systemctl start ssftpd
sudo systemctl status ssftpd
```

**macOS (launchd):**
```bash
sudo launchctl load /Library/LaunchDaemons/com.blburns.ssftpd.plist
sudo launchctl start com.blburns.ssftpd
```

**Windows:**
```cmd
sc create ssftpd binPath= "C:\Program Files\ssftpd\bin\ssftpd.exe"
sc start ssftpd
```

## Configuration

### Main Configuration File

The main configuration file (`ssftpd.conf`) supports both INI and JSON formats. Here's an example INI configuration:

```ini
# Global server settings
server_name = "Simple-Secure FTP Daemon"
server_version = "0.1.0"
enable_ssl = true
enable_virtual_hosts = true

# SSL Configuration
[ssl]
enabled = true
certificate_file = "/etc/ssftpd/ssl/server.crt"
private_key_file = "/etc/ssftpd/ssl/server.key"

# Connection settings
[connection]
bind_address = "0.0.0.0"
bind_port = 21
max_connections = 100

# Virtual hosts
[virtual_hosts.default]
hostname = "default"
document_root = "/var/ftp"
enabled = true

# Users
[users.admin]
username = "admin"
password_hash = "$2y$10$hashed_password"
home_directory = "/var/ftp/admin"
permissions = ["READ", "WRITE", "LIST", "UPLOAD", "DOWNLOAD"]
```

### User Management

#### Adding Users

```bash
# Add a new user
sudo ssftpd user add \
  --username john \
  --password secret \
  --home /var/ftp/john \
  --permissions READ,WRITE,LIST,UPLOAD,DOWNLOAD

# Add anonymous user
sudo ssftpd user add \
  --username anonymous \
  --home /var/ftp/public \
  --anonymous \
  --permissions READ,LIST,DOWNLOAD
```

#### User Permissions

Available permissions:
- `READ`: Read files and directories
- `WRITE`: Write/create files and directories
- `DELETE`: Delete files and directories
- `RENAME`: Rename files and directories
- `MKDIR`: Create directories
- `RMDIR`: Remove directories
- `LIST`: List directory contents
- `UPLOAD`: Upload files
- `DOWNLOAD`: Download files
- `APPEND`: Append to files
- `ADMIN`: Administrative operations

### Virtual Hosts

#### Adding Virtual Hosts

```bash
# Add a new virtual host
sudo ssftpd virtual add \
  --hostname ftp.example.com \
  --root /var/ftp/example \
  --ssl \
  --certificate /etc/ssl/certs/example.com.crt \
  --private-key /etc/ssl/private/example.com.key
```

#### Virtual Host Configuration

Each virtual host can have:
- Separate document root
- Individual SSL certificates
- Custom security settings
- User access restrictions
- Transfer rate limits

### SSL/TLS Configuration

#### Generating Self-Signed Certificates

```bash
# Generate certificate for a domain
sudo ssftpd ssl generate \
  --hostname ftp.example.com \
  --country US \
  --state California \
  --city San Francisco \
  --organization "Example Corp" \
  --email admin@example.com
```

#### Installing Certificates

```bash
# Install existing certificates
sudo ssftpd ssl install \
  --hostname ftp.example.com \
  --certificate /path/to/certificate.crt \
  --private-key /path/to/private.key \
  --ca-certificate /path/to/ca.crt
```

## Command Line Interface

### Server Commands

```bash
# Start the server
ssftpd start [--config FILE] [--daemon] [--foreground]

# Stop the server
ssftpd stop

# Restart the server
ssftpd restart

# Show server status
ssftpd status

# Reload configuration
ssftpd reload
```

### User Management Commands

```bash
# List all users
ssftpd user list

# Add user
ssftpd user add --username NAME --password PASS --home DIR

# Remove user
ssftpd user remove --username NAME

# Modify user
ssftpd user modify --username NAME --permissions READ,WRITE

# Change password
ssftpd user password --username NAME --password NEW_PASS
```

### Virtual Host Commands

```bash
# List virtual hosts
ssftpd virtual list

# Add virtual host
ssftpd virtual add --hostname DOMAIN --root DIR

# Remove virtual host
ssftpd virtual remove --hostname DOMAIN

# Enable/disable virtual host
ssftpd virtual enable --hostname DOMAIN
ssftpd virtual disable --hostname DOMAIN
```

### SSL Management Commands

```bash
# Generate certificate
ssftpd ssl generate --hostname DOMAIN

# Install certificate
ssftpd ssl install --hostname DOMAIN --cert FILE --key FILE

# Show SSL status
ssftpd ssl status

# Renew certificate
ssftpd ssl renew --hostname DOMAIN
```

## Development

### Building from Source

```bash
# Clone and setup
git clone https://github.com/ssftpd/ssftpd.git
cd ssftpd

# Install dependencies
make install-dev

# Build options
make debug      # Debug build
make release    # Release build
make test       # Run tests
make clean      # Clean build artifacts

# Package creation
make package    # Create packages for current platform
```

### Project Structure

```
ssftpd/
├── include/ssftpd/     # Header files
│   ├── ftp_server.hpp       # Main server class
│   ├── ftp_connection.hpp   # Connection handling
│   ├── ftp_user.hpp         # User management
│   ├── ftp_virtual_host.hpp # Virtual host support
│   ├── ftp_server_config.hpp # Configuration
│   ├── logger.hpp           # Logging system
│   └── platform.hpp         # Platform abstraction
├── src/                     # Source files
│   ├── core/               # Core implementation
│   ├── utils/              # Utility functions
│   └── main.cpp            # Main application
├── config/                  # Configuration files
├── tools/                   # Management tools
├── docs/                    # Documentation
├── scripts/                 # Build and deployment scripts
├── deployment/              # Deployment configurations
│   └── examples/
│       └── docker/          # Docker deployment examples
├── Dockerfile              # Multi-stage Docker build
├── docker-compose.yml      # Docker Compose orchestration
├── .dockerignore           # Docker build context optimization
├── CMakeLists.txt          # CMake build configuration
└── Makefile                # Make build system
```

## 🐳 Docker Infrastructure

ssftpd includes comprehensive Docker support for development, testing, and production deployment:

### Docker Features

- **Multi-stage builds** for different Linux distributions (Ubuntu, CentOS, Alpine)
- **Multi-architecture support** (x86_64, ARM64, ARMv7)
- **Development environment** with debugging tools and live code mounting
- **Production-ready runtime** with minimal footprint and security hardening
- **Health checks** and monitoring capabilities
- **Volume mounts** for configuration, logs, and FTP data

### Quick Docker Commands

```bash
# Development environment
docker-compose --profile dev up -d

# Production deployment
docker-compose --profile runtime up -d

# Build for all platforms
./scripts/build-docker.sh -d all

# Deploy with custom configuration
./scripts/deploy-docker.sh -p runtime -c ./config -l ./logs -d ./data
```

### Docker Ports

- **21/tcp** - FTP control port
- **990/tcp** - FTPS control port (SSL/TLS)
- **1024-65535/tcp** - Passive mode data ports

For complete Docker documentation, see [Docker Deployment Guide](docs/deployment/docker.md).

### Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

### Testing

```bash
# Run all tests
make test

# Run specific test suites
cd build && ctest -R "unit_tests"
cd build && ctest -R "integration_tests"

# Run with coverage (Linux only)
make coverage

# Run with memory checking (Linux only)
make memcheck
```

## Troubleshooting

### Common Issues

#### Server Won't Start

1. **Check configuration:**
```bash
ssftpd --test-config --config /etc/ssftpd/ssftpd.conf
```

2. **Check permissions:**
```bash
sudo chown -R ftp:ftp /var/ftp
sudo chmod 755 /var/ftp
```

3. **Check ports:**
```bash
sudo netstat -tlnp | grep :21
```

#### SSL Connection Issues

1. **Verify certificate files:**
```bash
openssl x509 -in /etc/ssftpd/ssl/server.crt -text -noout
```

2. **Check certificate permissions:**
```bash
sudo chmod 600 /etc/ssftpd/ssl/server.key
sudo chown ftp:ftp /etc/ssftpd/ssl/server.key
```

#### Performance Issues

1. **Check system resources:**
```bash
top
iostat
netstat -i
```

2. **Adjust configuration:**
```ini
[connection]
max_connections = 50
thread_pool_size = 4

[transfer]
buffer_size = 16384
use_sendfile = true
```

### Log Files

- **Main log**: `/var/log/ssftpd/ssftpd.log`
- **Access log**: `/var/log/ssftpd/access.log`
- **Error log**: `/var/log/ssftpd/error.log`

### Debug Mode

Enable debug logging:

```ini
[logging]
log_level = "DEBUG"
log_commands = true
log_transfers = true

# Development settings
debug_mode = true
verbose_logging = true
trace_commands = true
```

## Security Considerations

### Production Deployment

1. **Use strong SSL certificates**
2. **Enable chroot for users**
3. **Drop privileges to non-root user**
4. **Implement rate limiting**
5. **Restrict allowed commands**
6. **Use firewall rules**
7. **Regular security updates**

### Network Security

```bash
# Firewall rules (iptables)
sudo iptables -A INPUT -p tcp --dport 21 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 1024:65535 -j ACCEPT

# Or with ufw (Ubuntu)
sudo ufw allow 21/tcp
sudo ufw allow 1024:65535/tcp
```

### User Security

1. **Strong password policies**
2. **Limited permissions**
3. **Path restrictions**
4. **Connection limits**
5. **Session timeouts**

## Performance Tuning

### System Tuning

```bash
# Increase file descriptor limits
echo "* soft nofile 65536" >> /etc/security/limits.conf
echo "* hard nofile 65536" >> /etc/security/limits.conf

# Kernel parameters
echo "net.core.somaxconn = 65536" >> /etc/sysctl.conf
echo "net.ipv4.tcp_max_syn_backlog = 65536" >> /etc/sysctl.conf
sysctl -p
```

### Application Tuning

```ini
[connection]
max_connections = 200
backlog = 100
keep_alive = true
tcp_nodelay = true

[transfer]
buffer_size = 32768
use_sendfile = true
use_mmap = true

# Performance settings
thread_pool_size = 16
enable_compression = true
cache_size = 100MB
```

## Monitoring and Metrics

### Built-in Monitoring

```bash
# Show server statistics
ssftpd status

# Show performance metrics
ssftpd metrics

# Show connection information
ssftpd connections
```

### External Monitoring

- **Prometheus**: Metrics endpoint at `/metrics`
- **Grafana**: Dashboard templates included
- **Log aggregation**: Structured logging support
- **Health checks**: HTTP health endpoint

## License

This project is licensed under the Apache License, Version 2.0 - see the [LICENSE](LICENSE) file for details.

## Support

- **Documentation**: [docs/](docs/)
- **Issues**: [GitHub Issues](https://github.com/ssftpd/ssftpd/issues)
- **Discussions**: [GitHub Discussions](https://github.com/ssftpd/ssftpd/discussions)
- **Email**: SimpleDaemons

## Acknowledgments

- **OpenSSL**: SSL/TLS implementation
- **jsoncpp**: JSON parsing library
- **CMake**: Build system
- **FTP RFC 959**: Protocol specification

## Changelog

### v0.1.0 (Current)
- Initial release
- Core FTP server functionality
- SSL/TLS support
- Virtual hosting
- User management
- Multi-platform support
- Comprehensive configuration system
