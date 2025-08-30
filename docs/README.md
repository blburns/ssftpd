# ssftpd Documentation

Welcome to the comprehensive documentation for **Simple-Secure FTP Daemon (ssftpd)**, a secure, configurable, and feature-rich FTP server written in C++.

## 📚 Documentation Overview

This documentation is organized into several sections to help you find the information you need quickly and efficiently.

### 🚀 Getting Started
- **[Installation Guide](installation/README.md)** - Complete installation instructions for all platforms
- **[Quick Start Guide](getting-started/README.md)** - Get up and running in minutes
- **[Configuration Basics](configuration/README.md)** - Essential configuration concepts

### 👥 User Guides
- **[User Guide](user-guide/README.md)** - Complete user manual and reference
- **[Examples](examples/README.md)** - Practical examples and use cases
- **[Troubleshooting](user-guide/troubleshooting.md)** - Common issues and solutions

### 🔧 Development & API
- **[Development Guide](development/README.md)** - Building from source and contributing
- **[API Reference](api/README.md)** - Complete API documentation
- **[Architecture](development/architecture.md)** - System design and architecture

### 📖 Reference Materials
- **[Configuration Reference](configuration/reference.md)** - Complete configuration options
- **[Command Line Reference](user-guide/commands.md)** - All available commands
- **[Security Guide](user-guide/security.md)** - Security best practices

## 🎯 Quick Navigation

| What You Need | Start Here |
|---------------|------------|
| **Install ssftpd** | [Installation Guide](installation/README.md) |
| **Configure the server** | [Configuration Guide](configuration/README.md) |
| **Learn basic usage** | [Quick Start](getting-started/README.md) |
| **Find examples** | [Examples](examples/README.md) |
| **Troubleshoot issues** | [Troubleshooting](user-guide/troubleshooting.md) |
| **Build from source** | [Development Guide](development/README.md) |
| **API reference** | [API Documentation](api/README.md) |

## 🌟 Key Features

### 🔒 Security First
- **SSL/TLS encryption** for secure file transfers
- **User authentication** with multiple backends
- **Access control** and path restrictions
- **Chroot isolation** for enhanced security

### 🚀 High Performance
- **Multi-threaded architecture** for concurrent connections
- **Optimized I/O** with sendfile and memory mapping
- **Connection pooling** for efficient resource usage
- **Rate limiting** to prevent abuse

### 🏗️ Enterprise Ready
- **Virtual hosting** for multiple domains
- **Comprehensive logging** with rotation
- **Monitoring and metrics** for production use
- **Service integration** (systemd, launchd, Windows)

### 🔧 Developer Friendly
- **Modern C++17** codebase
- **Comprehensive testing** with Google Test
- **Cross-platform** support (Linux, macOS, Windows)
- **Extensible architecture** for custom features

## 📋 Prerequisites

Before using ssftpd, ensure you have:

- **C++17 Compiler**: GCC 7+, Clang 5+, or MSVC 2017+
- **CMake 3.16+**: Build system
- **OpenSSL**: SSL/TLS support
- **jsoncpp**: JSON configuration parsing

## 🚀 Quick Start

```bash
# Install from source
git clone https://github.com/ssftpd/ssftpd.git
cd ssftpd
make install-dev
make build
make install

# Or install from packages
sudo apt install ssftpd  # Ubuntu/Debian
brew install ssftpd      # macOS
```

## 📖 Documentation Versions

- **Current Version**: v0.1.0
- **Documentation**: Always up-to-date with latest release
- **API**: Covers all public interfaces
- **Examples**: Tested and verified

## 🤝 Contributing to Documentation

We welcome contributions to improve this documentation:

1. **Report Issues**: Found an error or unclear section?
2. **Suggest Improvements**: Have ideas for better organization?
3. **Submit Fixes**: Correct typos or add missing information
4. **Add Examples**: Share your use cases and configurations

See our [Contributing Guide](development/contributing.md) for details.

## 📞 Support & Community

- **Documentation Issues**: [GitHub Issues](https://github.com/ssftpd/ssftpd/issues)
- **Questions**: [GitHub Discussions](https://github.com/ssftpd/ssftpd/discussions)
- **Security Issues**: [Security Policy](https://github.com/ssftpd/ssftpd/security/policy)

## 📄 License

This documentation is licensed under the [Apache License, Version 2.0](LICENSE).

---

**Ready to get started?** Begin with the [Installation Guide](installation/README.md) or jump to [Quick Start](getting-started/README.md) if you're already familiar with FTP servers.
