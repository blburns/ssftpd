# System Integration Configuration

This directory contains system-level configuration files for integrating ssftpd with various operating systems and services.

## 📁 Directory Structure

```
etc/
├── README.md                    # This file
├── systemd/                     # systemd service files (Linux)
│   ├── ssftpd.service          # Main service file
│   ├── ssftpd@.service         # Template for multiple instances
│   └── ssftpd.target           # Target for managing instances
├── launchd/                     # macOS LaunchDaemon files
│   └── com.ssftpd.ssftpd.plist # macOS service configuration
├── init.d/                      # SysV init scripts (legacy Linux)
├── logrotate.d/                 # Log rotation configuration
└── rsyslog.d/                   # rsyslog configuration
```

## 🐧 Linux systemd Integration

### Service Management

```bash
# Enable and start the service
sudo systemctl enable ssftpd
sudo systemctl start ssftpd

# Check service status
sudo systemctl status ssftpd

# Reload configuration
sudo systemctl reload ssftpd

# Stop the service
sudo systemctl stop ssftpd

# View logs
sudo journalctl -u ssftpd -f
```

### Multiple Instances

```bash
# Start additional instances
sudo systemctl start ssftpd@instance1
sudo systemctl start ssftpd@instance2

# Enable instances
sudo systemctl enable ssftpd@instance1
sudo systemctl enable ssftpd@instance2

# Manage all instances
sudo systemctl start ssftpd.target
sudo systemctl stop ssftpd.target
```

### Service Configuration

The systemd service files include:

- **Security hardening**: NoNewPrivileges, ProtectSystem, etc.
- **Resource limits**: File descriptors, process limits
- **Directory management**: Runtime, state, logs, configuration
- **Restart policies**: Automatic restart on failure

## 🍎 macOS Integration

### Install LaunchDaemon

```bash
# Copy the plist file
sudo cp etc/launchd/com.ssftpd.ssftpd.plist /Library/LaunchDaemons/

# Set permissions
sudo chown root:wheel /Library/LaunchDaemons/com.ssftpd.ssftpd.plist
sudo chmod 644 /Library/LaunchDaemons/com.ssftpd.ssftpd.plist

# Load the service
sudo launchctl load /Library/LaunchDaemons/com.ssftpd.ssftpd.plist
```

### Service Management

```bash
# Start the service
sudo launchctl start com.ssftpd.ssftpd

# Stop the service
sudo launchctl stop com.ssftpd.ssftpd

# Unload the service
sudo launchctl unload /Library/LaunchDaemons/com.ssftpd.ssftpd.plist

# Check if running
sudo launchctl list | grep ssftpd
```

## 📊 Log Management

### Logrotate Configuration

The logrotate configuration:

- Rotates logs daily
- Keeps 52 weeks of general logs
- Keeps 30 days of access/error/SSL logs
- Compresses old logs
- Reloads ssftpd after rotation
- Runs as ssftpd user for security

### Manual Log Rotation

```bash
# Test logrotate configuration
sudo logrotate -d /etc/logrotate.d/ssftpd

# Force log rotation
sudo logrotate -f /etc/logrotate.d/ssftpd

# Check logrotate status
sudo cat /var/lib/logrotate/status | grep ssftpd
```

### Log Locations

```
/var/log/ssftpd/
├── ssftpd.log          # Main application log
├── access.log          # Access log
├── error.log           # Error log
├── ssl.log             # SSL/TLS log
└── ssl-renewal.log     # SSL certificate renewal log
```

## 🔧 Configuration Integration

### Service Configuration

The service files reference the deployment configuration:

```bash
# Main configuration
--config /etc/ssftpd/deployment/ssftpd.conf

# Instance-specific configuration
--config /etc/ssftpd/deployment/ssftpd.conf --instance instance1
```

### Directory Structure

```
/usr/local/etc/ssftpd/          # macOS
/etc/ssftpd/                    # Linux
├── deployment/                 # Main configuration
├── ssl/                       # SSL certificates
└── users/                     # User configurations
```

## 🚀 Quick Setup

### Linux (systemd)

```bash
# 1. Copy service files
sudo cp etc/systemd/*.service /etc/systemd/system/
sudo cp etc/systemd/*.target /etc/systemd/system/

# 2. Reload systemd
sudo systemctl daemon-reload

# 3. Enable and start
sudo systemctl enable ssftpd
sudo systemctl start ssftpd

# 4. Check status
sudo systemctl status ssftpd
```

### macOS (LaunchDaemon)

```bash
# 1. Copy plist file
sudo cp etc/launchd/com.ssftpd.ssftpd.plist /Library/LaunchDaemons/

# 2. Set permissions
sudo chown root:wheel /Library/LaunchDaemons/com.ssftpd.ssftpd.plist
sudo chmod 644 /Library/LaunchDaemons/com.ssftpd.ssftpd.plist

# 3. Load service
sudo launchctl load /Library/LaunchDaemons/com.ssftpd.ssftpd.plist

# 4. Start service
sudo launchctl start com.ssftpd.ssftpd
```

### Log Rotation

```bash
# 1. Copy logrotate configuration
sudo cp etc/logrotate.d/ssftpd /etc/logrotate.d/

# 2. Test configuration
sudo logrotate -d /etc/logrotate.d/ssftpd

# 3. Set up cron job (if not using systemd timer)
sudo crontab -e
# Add: 0 0 * * * /usr/sbin/logrotate /etc/logrotate.d/ssftpd
```

## 🔍 Troubleshooting

### Service Won't Start

```bash
# Check service status
sudo systemctl status ssftpd

# View detailed logs
sudo journalctl -u ssftpd -n 50

# Check configuration
sudo ssftpd --test-config --config /etc/ssftpd/deployment/ssftpd.conf

# Check file permissions
sudo ls -la /etc/ssftpd/
sudo ls -la /var/log/ssftpd/
```

### Permission Issues

```bash
# Fix directory permissions
sudo chown -R ssftpd:ssftpd /etc/ssftpd/
sudo chown -R ssftpd:ssftpd /var/log/ssftpd/
sudo chown -R ssftpd:ssftpd /var/lib/ssftpd/

# Fix file permissions
sudo chmod 600 /etc/ssftpd/ssl/*.key
sudo chmod 644 /etc/ssftpd/ssl/*.crt
```

### Port Conflicts

```bash
# Check what's using the ports
sudo netstat -tlnp | grep :21
sudo netstat -tlnp | grep :990

# Stop conflicting services
sudo systemctl stop vsftpd
sudo systemctl stop pure-ftpd
```

## 📚 Additional Resources

### systemd Documentation
- [systemd.service(5)](https://www.freedesktop.org/software/systemd/man/systemd.service.html)
- [systemd.target(5)](https://www.freedesktop.org/software/systemd/man/systemd.target.html)

### macOS Documentation
- [launchd.plist(5)](https://developer.apple.com/library/archive/documentation/MacOSX/Conceptual/BPSystemStartup/Chapters/CreatingLaunchdJobs.html)

### Logrotate Documentation
- [logrotate(8)](https://linux.die.net/man/8/logrotate)

---

**Next Steps**: 
1. Copy the appropriate service files for your system
2. Configure the service to start automatically
3. Set up log rotation
4. Test the service integration
5. Configure firewall rules if needed
