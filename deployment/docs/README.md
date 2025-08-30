# Deployment Configuration

This directory contains the deployment configuration structure for ssftpd, inspired by Apache2 and Pure-FTPD configuration management.

## 📁 Directory Structure

```
deployment/
├── README.md                    # This file
├── sites-available/             # Available virtual host configurations
├── sites-enabled/               # Enabled virtual host configurations (symlinks)
├── modules-available/           # Available module configurations
├── modules-enabled/             # Enabled module configurations (symlinks)
├── conf.d/                      # Additional configuration files
├── ssftpd.conf                  # Main server configuration
├── modules.conf                 # Module loading configuration
└── sites.conf                   # Virtual host configuration
```

## 🔧 Configuration Management

### Virtual Hosts (Apache2 Style)

Virtual hosts are managed using the `sites-available` and `sites-enabled` directories:

```bash
# Enable a virtual host
sudo ln -s /etc/ssftpd/sites-available/example.com.conf /etc/ssftpd/sites-enabled/

# Disable a virtual host
sudo rm /etc/ssftpd/sites-enabled/example.com.conf

# List enabled sites
ls -la /etc/ssftpd/sites-enabled/

# List available sites
ls -la /etc/ssftpd/sites-available/
```

### Modules (Pure-FTPD Style)

Modules are managed using the `modules-available` and `modules-enabled` directories:

```bash
# Enable a module
sudo ln -s /etc/ssftpd/modules-available/ssl.conf /etc/ssftpd/modules-enabled/

# Disable a module
sudo rm /etc/ssftpd/modules-enabled/ssl.conf

# List enabled modules
ls -la /etc/ssftpd/modules-enabled/

# List available modules
ls -la /etc/ssftpd/modules-available/
```

## 📝 Configuration Files

### Main Configuration (`ssftpd.conf`)

The main configuration file that includes all enabled configurations:

```json
{
  "server": {
    "name": "Simple-Secure FTP Daemon",
    "version": "0.1.0",
    "listen_address": "0.0.0.0",
    "listen_port": 21
  },
  "includes": [
    "conf.d/*.conf",
    "modules-enabled/*.conf",
    "sites-enabled/*.conf"
  ]
}
```

### Module Configuration (`modules.conf`)

Controls which modules are loaded and their order:

```json
{
  "modules": {
    "ssl": {
      "enabled": true,
      "priority": 10,
      "config_file": "modules-enabled/ssl.conf"
    },
    "auth": {
      "enabled": true,
      "priority": 20,
      "config_file": "modules-enabled/auth.conf"
    },
    "rate_limit": {
      "enabled": true,
      "priority": 30,
      "config_file": "modules-enabled/rate_limit.conf"
    }
  }
}
```

### Site Configuration (`sites.conf`)

Manages virtual host configurations:

```json
{
  "sites": {
    "default": {
      "enabled": true,
      "config_file": "sites-enabled/default.conf"
    },
    "example.com": {
      "enabled": true,
      "config_file": "sites-enabled/example.com.conf"
    },
    "ftp.example.com": {
      "enabled": false,
      "config_file": "sites-available/ftp.example.com.conf"
    }
  }
}
```

## 🚀 Quick Start

### 1. Enable Default Configuration

```bash
# Enable default site
sudo ln -s /etc/ssftpd/sites-available/default.conf /etc/ssftpd/sites-enabled/

# Enable essential modules
sudo ln -s /etc/ssftpd/modules-available/ssl.conf /etc/ssftpd/modules-enabled/
sudo ln -s /etc/ssftpd/modules-available/auth.conf /etc/ssftpd/modules-enabled/

# Reload configuration
sudo ssftpd reload
```

### 2. Add New Virtual Host

```bash
# Create configuration
sudo cp /etc/ssftpd/sites-available/example.com.conf /etc/ssftpd/sites-available/mysite.com.conf

# Edit configuration
sudo nano /etc/ssftpd/sites-available/mysite.com.conf

# Enable site
sudo ln -s /etc/ssftpd/sites-available/mysite.com.conf /etc/ssftpd/sites-enabled/

# Reload configuration
sudo ssftpd reload
```

### 3. Enable/Disable Features

```bash
# Enable SSL
sudo ln -s /etc/ssftpd/modules-available/ssl.conf /etc/ssftpd/modules-enabled/

# Enable rate limiting
sudo ln -s /etc/ssftpd/modules-available/rate_limit.conf /etc/ssftpd/modules-enabled/

# Disable module
sudo rm /etc/ssftpd/modules-enabled/rate_limit.conf

# Reload configuration
sudo ssftpd reload
```

## 🔍 Configuration Validation

### Test Configuration

```bash
# Test main configuration
ssftpd --test-config --config /etc/ssftpd/deployment/ssftpd.conf

# Test specific module
ssftpd --test-module --module ssl --config /etc/ssftpd/deployment/modules-enabled/ssl.conf

# Test specific site
ssftpd --test-site --site example.com --config /etc/ssftpd/deployment/sites-enabled/example.com.conf
```

### Validate Symlinks

```bash
# Check for broken symlinks
find /etc/ssftpd/sites-enabled/ -type l -exec test ! -e {} \; -print
find /etc/ssftpd/modules-enabled/ -type l -exec test ! -e {} \; -print

# Verify symlink targets
ls -la /etc/ssftpd/sites-enabled/
ls -la /etc/ssftpd/modules-enabled/
```

## 📚 Best Practices

### 1. Configuration Organization

- Keep configurations modular and focused
- Use descriptive filenames
- Document custom configurations
- Version control your configurations

### 2. Security

- Restrict access to configuration files
- Use proper file permissions (600 for configs)
- Validate configurations before enabling
- Test in staging environment first

### 3. Maintenance

- Regular configuration backups
- Monitor for configuration errors
- Document changes and reasons
- Use configuration management tools

## 🛠️ Management Scripts

### Enable/Disable Sites

```bash
# Enable site
sudo ssftpd-site enable example.com

# Disable site
sudo ssftpd-site disable example.com

# List sites
ssftpd-site list

# Show site status
ssftpd-site status example.com
```

### Enable/Disable Modules

```bash
# Enable module
sudo ssftpd-module enable ssl

# Disable module
sudo ssftpd-module disable ssl

# List modules
ssftpd-module list

# Show module status
ssftpd-module status ssl
```

## 🔄 Migration from Traditional Config

### From Single Config File

```bash
# Backup existing configuration
sudo cp /etc/ssftpd/ssftpd.conf /etc/ssftpd/ssftpd.conf.backup

# Extract modules to separate files
sudo ssftpd-config extract-modules /etc/ssftpd/ssftpd.conf

# Extract virtual hosts to separate files
sudo ssftpd-config extract-sites /etc/ssftpd/ssftpd.conf

# Enable extracted configurations
sudo ssftpd-config enable-all
```

### From Multiple Config Files

```bash
# Import existing configurations
sudo ssftpd-config import /path/to/existing/configs/

# Validate imported configurations
sudo ssftpd-config validate

# Enable imported configurations
sudo ssftpd-config enable-imported
```

## 📞 Support

For help with deployment configuration:

- Check configuration examples in each directory
- Review the main documentation
- Open an issue on GitHub
- Join community discussions

---

**Next Steps**: Explore the configuration examples in each directory to understand how to structure your deployments.
