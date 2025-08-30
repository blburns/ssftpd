# Packaging Guide

This directory contains packaging configurations for building distribution packages for multiple platforms and package managers.

## 📦 Supported Package Formats

### Linux
- **RPM** - Red Hat, CentOS, Fedora, RHEL, SUSE
- **DEB** - Debian, Ubuntu, Linux Mint
- **Generic** - Source tarballs, manual installation

### Windows
- **NSIS Installer** - Windows executable installer (.exe)
- **MSI** - Windows Installer package (planned)
- **Chocolatey** - Windows package manager (planned)

### macOS
- **Homebrew** - macOS package manager (planned)
- **DMG** - macOS disk image installer (planned)

## 🏗️ Building Packages

### Prerequisites

#### For RPM Packages
```bash
# CentOS/RHEL/Fedora
sudo yum install rpm-build rpmdevtools
# or
sudo dnf install rpm-build rpmdevtools

# Ubuntu/Debian
sudo apt install rpm rpm-build
```

#### For DEB Packages
```bash
# Ubuntu/Debian
sudo apt install build-essential devscripts debhelper dh-make

# CentOS/RHEL/Fedora
sudo yum install rpm-build rpmdevtools
# or
sudo dnf install rpm-build rpmdevtools
```

#### For Windows Installers
```bash
# Install NSIS (Nullsoft Scriptable Install System)
# Download from: https://nsis.sourceforge.io/Download
# or use Chocolatey: choco install nsis

# Install NSSM (Non-Sucking Service Manager)
# Download from: https://nssm.cc/
```

### Building RPM Packages

#### 1. Setup RPM Build Environment
```bash
# Create RPM build structure
rpmdev-setuptree

# Copy source to RPM build directory
cp ssftpd-0.1.0.tar.gz ~/rpmbuild/SOURCES/
cp packaging/rpm/ssftpd.spec ~/rpmbuild/SPECS/
```

#### 2. Build RPM Package
```bash
# Build the package
rpmbuild -ba ~/rpmbuild/SPECS/ssftpd.spec

# The package will be created in ~/rpmbuild/RPMS/
```

#### 3. Install RPM Package
```bash
# Install the package
sudo rpm -ivh ~/rpmbuild/RPMS/x86_64/ssftpd-0.1.0-1.el8.x86_64.rpm

# Or upgrade existing installation
sudo rpm -Uvh ~/rpmbuild/RPMS/x86_64/ssftpd-0.1.0-1.el8.x86_64.rpm
```

### Building DEB Packages

#### 1. Setup DEB Build Environment
```bash
# Create package directory
mkdir ssftpd-0.1.0
cd ssftpd-0.1.0

# Copy source files
cp -r ../src ../include ../cmake ../deployment ../etc ../docs ./
cp ../CMakeLists.txt ../LICENSE ../README.md ./

# Copy Debian packaging files
cp -r ../packaging/deb/debian ./
```

#### 2. Build DEB Package
```bash
# Build the package
debuild -b -us -uc

# The package will be created in the parent directory
```

#### 3. Install DEB Package
```bash
# Install the package
sudo dpkg -i ../ssftpd_0.1.0_amd64.deb

# Fix dependencies if needed
sudo apt-get install -f
```

### Building Windows Installers

#### 1. Prepare Windows Build
```bash
# Cross-compile for Windows (if building on Linux/macOS)
# Or build natively on Windows

# Ensure all required files are present:
# - ssftpd.exe
# - ssftpd-site.exe
# - ssftpd-module.exe
# - *.dll files
# - deployment/ directory
# - etc/windows/ directory
# - docs/ directory
# - nssm.exe
# - LICENSE file
```

#### 2. Build NSIS Installer
```bash
# Run NSIS compiler
makensis packaging/windows/installer.nsi

# The installer will be created as ssftpd-0.1.0-windows-x64.exe
```

#### 3. Install on Windows
- Run the installer as Administrator
- Follow the installation wizard
- The service will be automatically installed and started

## 📁 Package Contents

### RPM Package Structure
```
ssftpd-0.1.0-1.el8.x86_64.rpm
├── /usr/sbin/ssftpd
├── /usr/sbin/ssftpd-site
├── /usr/sbin/ssftpd-module
├── /usr/lib64/libssftpd.so
├── /usr/include/ssftpd/
├── /etc/ssftpd/
├── /etc/systemd/system/ssftpd.service
├── /etc/rc.d/init.d/ssftpd
├── /var/ftp/
├── /var/log/ssftpd/
└── /var/lib/ssftpd/
```

### DEB Package Structure
```
ssftpd_0.1.0_amd64.deb
├── /usr/sbin/ssftpd
├── /usr/sbin/ssftpd-site
├── /usr/sbin/ssftpd-module
├── /usr/lib/x86_64-linux-gnu/libssftpd.so
├── /usr/include/ssftpd/
├── /etc/ssftpd/
├── /etc/systemd/system/ssftpd.service
├── /etc/init.d/ssftpd
├── /var/ftp/
├── /var/log/ssftpd/
└── /var/lib/ssftpd/
```

### Windows Installer Structure
```
ssftpd-0.1.0-windows-x64.exe
├── C:\Program Files\ssftpd\
│   ├── ssftpd.exe
│   ├── ssftpd-site.exe
│   ├── ssftpd-module.exe
│   ├── *.dll
│   ├── etc\deployment\
│   ├── etc\windows\
│   ├── docs\
│   ├── logs\
│   ├── var\
│   └── ssl\
└── Windows Service: ssftpd
```

## 🔧 Package Configuration

### Service Management

#### systemd (Modern Linux)
```bash
# Enable and start service
sudo systemctl enable ssftpd
sudo systemctl start ssftpd

# Check status
sudo systemctl status ssftpd

# Reload configuration
sudo systemctl reload ssftpd
```

#### SysV init (Traditional Linux)
```bash
# Start service
sudo /etc/init.d/ssftpd start

# Check status
sudo /etc/init.d/ssftpd status

# Reload configuration
sudo /etc/init.d/ssftpd reload
```

#### Windows Service
```bash
# Start service
net start ssftpd

# Stop service
net stop ssftpd

# Check status
sc query ssftpd
```

### Configuration Management

#### Enable/Disable Sites
```bash
# Enable site
sudo ssftpd-site enable example.com

# Disable site
sudo ssftpd-site disable example.com

# List sites
ssftpd-site list
```

#### Enable/Disable Modules
```bash
# Enable module
sudo ssftpd-module enable ssl

# Disable module
sudo ssftpd-module disable rate_limit

# List modules
ssftpd-module list
```

## 🚀 Quick Package Installation

### Ubuntu/Debian
```bash
# Add repository (when available)
# sudo apt-add-repository ppa:ssftpd/stable
# sudo apt update

# Install package
sudo apt install ssftpd

# Start service
sudo systemctl start ssftpd
sudo systemctl enable ssftpd
```

### CentOS/RHEL/Fedora
```bash
# Install package
sudo yum install ssftpd
# or
sudo dnf install ssftpd

# Start service
sudo systemctl start ssftpd
sudo systemctl enable ssftpd
```

### Windows
```bash
# Download and run installer as Administrator
# The service will be automatically installed and started
```

## 📊 Package Verification

### Verify Package Contents
```bash
# RPM
rpm -qlp ssftpd-0.1.0-1.el8.x86_64.rpm

# DEB
dpkg -c ssftpd_0.1.0_amd64.deb

# Windows
# Use NSIS installer to view contents
```

### Verify Installation
```bash
# Check if service is running
sudo systemctl status ssftpd

# Check configuration
ssftpd --test-config --config /etc/ssftpd/deployment/ssftpd.conf

# Check ports
sudo netstat -tlnp | grep :21
sudo netstat -tlnp | grep :990
```

## 🔍 Troubleshooting

### Common Issues

#### Package Installation Fails
```bash
# Check dependencies
sudo apt-get install -f  # Debian/Ubuntu
sudo yum check  # CentOS/RHEL

# Check package integrity
rpm -K package.rpm
dpkg -I package.deb
```

#### Service Won't Start
```bash
# Check logs
sudo journalctl -u ssftpd -f  # systemd
sudo tail -f /var/log/ssftpd/ssftpd.log

# Check configuration
ssftpd --test-config --config /etc/ssftpd/deployment/ssftpd.conf

# Check permissions
sudo ls -la /etc/ssftpd/
sudo ls -la /var/log/ssftpd/
```

#### Port Conflicts
```bash
# Check what's using the ports
sudo netstat -tlnp | grep :21
sudo netstat -tlnp | grep :990

# Stop conflicting services
sudo systemctl stop vsftpd
sudo systemctl stop pure-ftpd
```

## 📚 Additional Resources

### Package Manager Documentation
- [RPM Packaging Guide](https://rpm-packaging-guide.github.io/)
- [Debian Policy Manual](https://www.debian.org/doc/debian-policy/)
- [NSIS Documentation](https://nsis.sourceforge.io/Docs/)

### Distribution-Specific Guides
- [Fedora Packaging Guidelines](https://docs.fedoraproject.org/en-US/packaging-guidelines/)
- [Ubuntu Packaging Guide](https://packaging.ubuntu.com/html/)
- [CentOS Packaging Guidelines](https://wiki.centos.org/HowTos/PackageManagement/)

### Community Support
- [GitHub Issues](https://github.com/ssftpd/ssftpd/issues)
- [Community Forum](https://community.ssftpd.org)
- [Discord Server](https://discord.gg/ssftpd)

---

**Next Steps**: 
1. Choose your target platform and package format
2. Install the required build tools
3. Build and test the package
4. Install and verify the package
5. Configure and customize ssftpd for your needs
