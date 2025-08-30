# Installation Guide

This guide covers installing ssftpd on all supported platforms: Linux, macOS, and Windows.

## 📋 Prerequisites

### System Requirements

- **Operating System**: Linux (kernel 3.10+), macOS 12.0+, or Windows 10/11
- **Architecture**: x86_64, ARM64 (Linux/macOS), or ARM64 (Windows)
- **Memory**: Minimum 512MB RAM, recommended 2GB+
- **Storage**: Minimum 100MB for installation, additional space for logs and data
- **Network**: Network interface for FTP connections

### Software Dependencies

#### Required Dependencies
- **C++17 Compiler**: GCC 7+, Clang 5+, or MSVC 2017+
- **CMake 3.16+**: Build system
- **OpenSSL 1.1.1+**: SSL/TLS support
- **jsoncpp**: JSON configuration parsing
- **pthread**: Threading support (Linux/macOS)

#### Optional Dependencies
- **PAM**: Pluggable Authentication Modules (Linux)
- **LDAP**: Lightweight Directory Access Protocol
- **SQLite**: Local user database
- **MySQL/PostgreSQL**: External user database

## 🚀 Quick Installation

### From Pre-built Packages (Recommended)

#### Ubuntu/Debian
```bash
# Add repository (if available)
sudo apt update
sudo apt install ssftpd

# Or install from .deb package
wget https://github.com/ssftpd/ssftpd/releases/latest/download/ssftpd_amd64.deb
sudo dpkg -i ssftpd_amd64.deb
sudo apt-get install -f  # Fix any dependency issues
```

#### CentOS/RHEL/Fedora
```bash
# Install from .rpm package
sudo yum install https://github.com/ssftpd/ssftpd/releases/latest/download/ssftpd_x86_64.rpm

# Or for newer systems
sudo dnf install https://github.com/ssftpd/ssftpd/releases/latest/download/ssftpd_x86_64.rpm
```

#### macOS
```bash
# Using Homebrew
brew install ssftpd

# Or install from .pkg package
curl -LO https://github.com/ssftpd/ssftpd/releases/latest/download/ssftpd_macos.pkg
sudo installer -pkg ssftpd_macos.pkg -target /
```

#### Windows
```powershell
# Using Chocolatey
choco install ssftpd

# Or download and run .msi installer
# Download from: https://github.com/ssftpd/ssftpd/releases/latest/download/ssftpd_windows.msi
```

### From Source Code

#### 1. Clone the Repository
```bash
git clone https://github.com/ssftpd/ssftpd.git
cd ssftpd
```

#### 2. Install Development Dependencies

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential cmake libssl-dev libjsoncpp-dev git
```

**CentOS/RHEL/Fedora:**
```bash
sudo yum groupinstall "Development Tools"
sudo yum install cmake openssl-devel jsoncpp-devel git
```

**macOS:**
```bash
brew install cmake openssl jsoncpp
```

**Windows:**
```powershell
# Install Visual Studio Build Tools or Visual Studio Community
# Install CMake from https://cmake.org/download/
# Install vcpkg for dependencies
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat
./vcpkg install openssl jsoncpp
```

#### 3. Build and Install
```bash
# Build the project
make build

# Install system-wide
sudo make install

# Or install to custom directory
make install DESTDIR=/opt/ssftpd
```

## 🔧 Platform-Specific Installation

### Linux Installation

#### Systemd Service Setup
```bash
# Enable and start the service
sudo systemctl enable ssftpd
sudo systemctl start ssftpd

# Check status
sudo systemctl status ssftpd

# View logs
sudo journalctl -u ssftpd -f
```

#### Firewall Configuration
```bash
# UFW (Ubuntu)
sudo ufw allow 21/tcp
sudo ufw allow 30000:31000/tcp  # Passive mode ports

# firewalld (CentOS/RHEL)
sudo firewall-cmd --permanent --add-service=ftp
sudo firewall-cmd --permanent --add-port=30000-31000/tcp
sudo firewall-cmd --reload

# iptables
sudo iptables -A INPUT -p tcp --dport 21 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 30000:31000 -j ACCEPT
```

#### SELinux Configuration (RHEL/CentOS)
```bash
# Allow FTP through SELinux
sudo setsebool -P ftpd_full_access 1
sudo setsebool -P ftpd_use_passive_mode 1
```

### macOS Installation

#### LaunchDaemon Setup
```bash
# Create LaunchDaemon plist
sudo cp /usr/local/share/ssftpd/org.ssftpd.plist /Library/LaunchDaemons/

# Load and start the service
sudo launchctl load /Library/LaunchDaemons/org.ssftpd.plist
sudo launchctl start org.ssftpd

# Check status
sudo launchctl list | grep ssftpd
```

#### Firewall Configuration
```bash
# Allow incoming FTP connections
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --add /usr/local/bin/ssftpd
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --unblock /usr/local/bin/ssftpd
```

### Windows Installation

#### Windows Service Setup
```powershell
# Install as Windows Service
sc create ssftpd binPath= "C:\Program Files\ssftpd\bin\ssftpd.exe --service"
sc description ssftpd "Simple-Secure FTP Daemon"
sc start ssftpd

# Or use the installer which sets this up automatically
```

#### Windows Firewall
```powershell
# Allow FTP through Windows Firewall
netsh advfirewall firewall add rule name="ssftpd FTP" dir=in action=allow protocol=TCP localport=21
netsh advfirewall firewall add rule name="ssftpd Passive" dir=in action=allow protocol=TCP localport=30000-31000
```

## 📁 Installation Directory Structure

After installation, the following directory structure is created:

```
/etc/ssftpd/           # Configuration files
├── ssftpd.conf        # Main configuration
├── users/             # User definitions
├── virtual-hosts/     # Virtual host configurations
└── ssl/               # SSL certificates

/usr/local/bin/        # Executables
├── ssftpd             # Main server binary
├── ssftpd-ctl         # Control utility
└── ssftpd-user        # User management utility

/usr/local/share/ssftpd/ # Shared files
├── examples/          # Example configurations
├── scripts/           # Utility scripts
└── templates/         # Configuration templates

/var/log/ssftpd/       # Log files
├── ssftpd.log         # Main server log
├── access.log         # Access log
└── error.log          # Error log

/var/ftp/              # Default FTP root directory
└── public/            # Public files
```

## ⚙️ Post-Installation Configuration

### 1. Initial Configuration
```bash
# Copy example configuration
sudo cp /etc/ssftpd/ssftpd.conf.example /etc/ssftpd/ssftpd.conf

# Edit configuration
sudo nano /etc/ssftpd/ssftpd.conf
```

### 2. Create FTP User
```bash
# Create FTP user account
sudo ssftpd user add --username ftpuser --password securepass --home /var/ftp/ftpuser

# Or use the management script
sudo tools/manage-users.sh add ftpuser securepass /var/ftp/ftpuser
```

### 3. Set Up SSL (Optional but Recommended)
```bash
# Generate self-signed certificate
sudo ssftpd ssl generate --hostname localhost

# Or use Let's Encrypt
sudo ssftpd ssl install --hostname yourdomain.com --cert /etc/letsencrypt/live/yourdomain.com/fullchain.pem --key /etc/letsencrypt/live/yourdomain.com/privkey.pem
```

### 4. Test Configuration
```bash
# Test configuration file
ssftpd --test-config --config /etc/ssftpd/ssftpd.conf

# Start server in foreground for testing
ssftpd --config /etc/ssftpd/ssftpd.conf --foreground
```

## 🔍 Verification

### Check Installation
```bash
# Verify binary installation
which ssftpd
ssftpd --version

# Check service status
sudo systemctl status ssftpd  # Linux
sudo launchctl list | grep ssftpd  # macOS
sc query ssftpd  # Windows
```

### Test FTP Connection
```bash
# Test local connection
ftp localhost

# Or use command line client
ssftpd-ctl test-connection localhost 21
```

## 🚨 Troubleshooting

### Common Installation Issues

#### Missing Dependencies
```bash
# Check for missing libraries
ldd /usr/local/bin/ssftpd

# Install missing packages
sudo apt install libssl1.1 libjsoncpp1  # Ubuntu/Debian
sudo yum install openssl jsoncpp        # CentOS/RHEL
```

#### Permission Issues
```bash
# Fix ownership
sudo chown -R ftp:ftp /var/ftp
sudo chmod -R 755 /var/ftp

# Fix configuration permissions
sudo chown root:root /etc/ssftpd/ssftpd.conf
sudo chmod 600 /etc/ssftpd/ssftpd.conf
```

#### Port Already in Use
```bash
# Check what's using port 21
sudo netstat -tlnp | grep :21
sudo lsof -i :21

# Kill conflicting process or change port in configuration
```

### Getting Help

- **Check logs**: `/var/log/ssftpd/ssftpd.log`
- **Configuration errors**: `ssftpd --test-config`
- **Service issues**: Check system service status
- **Community support**: [GitHub Issues](https://github.com/ssftpd/ssftpd/issues)

## 🔄 Upgrading

### Package Manager Upgrade
```bash
# Ubuntu/Debian
sudo apt update && sudo apt upgrade ssftpd

# CentOS/RHEL
sudo yum update ssftpd

# macOS
brew upgrade ssftpd
```

### Source Code Upgrade
```bash
# Pull latest changes
git pull origin main

# Rebuild and reinstall
make clean
make build
sudo make install

# Restart service
sudo systemctl restart ssftpd  # Linux
sudo launchctl restart org.ssftpd  # macOS
```

## 📚 Next Steps

After successful installation:

1. **Configure the server**: See [Configuration Guide](../configuration/README.md)
2. **Set up users**: See [User Management](../user-guide/user-management.md)
3. **Enable SSL**: See [SSL Configuration](../configuration/ssl.md)
4. **Configure virtual hosts**: See [Virtual Hosting](../configuration/virtual-hosts.md)

---

**Need help?** Check our [Troubleshooting Guide](../user-guide/troubleshooting.md) or open an [issue on GitHub](https://github.com/ssftpd/ssftpd/issues).
