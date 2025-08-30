# User Guide

This comprehensive guide covers all aspects of using ssftpd, from basic operations to advanced features and troubleshooting.

## 🚀 Quick Start

### Starting the Server

```bash
# Start with default configuration
sudo ssftpd start

# Start with custom configuration
sudo ssftpd start --config /etc/ssftpd/ssftpd.conf

# Start in foreground mode (for testing)
sudo ssftpd --config /etc/ssftpd/ssftpd.conf --foreground

# Start as daemon
sudo ssftpd --daemon start --config /etc/ssftpd/ssftpd.conf
```

### Basic Commands

```bash
# Check server status
ssftpd status

# Stop the server
sudo ssftpd stop

# Restart the server
sudo ssftpd restart

# Reload configuration
sudo ssftpd reload

# Check server version
ssftpd --version
```

## 👥 User Management

### Creating Users

```bash
# Create a new user
sudo ssftpd user add \
  --username john \
  --password securepass123 \
  --home /var/ftp/john \
  --permissions READ,WRITE,UPLOAD,DOWNLOAD

# Create user with specific limits
sudo ssftpd user add \
  --username limited \
  --password pass123 \
  --home /var/ftp/limited \
  --permissions READ,DOWNLOAD \
  --max-connections 2 \
  --max-file-size 100MB \
  --transfer-rate 5MB/s

# Create admin user
sudo ssftpd user add \
  --username admin \
  --password adminpass \
  --home /var/ftp/admin \
  --permissions READ,WRITE,DELETE,UPLOAD,DOWNLOAD,ADMIN \
  --max-connections 10
```

### Managing Users

```bash
# List all users
ssftpd user list

# Get user details
ssftpd user info --username john

# Modify user permissions
sudo ssftpd user modify \
  --username john \
  --permissions READ,WRITE,UPLOAD,DOWNLOAD,DELETE

# Change user password
sudo ssftpd user password \
  --username john \
  --password newpassword123

# Disable user
sudo ssftpd user disable --username john

# Enable user
sudo ssftpd user enable --username john

# Remove user
sudo ssftpd user remove --username john
```

### User Types and Permissions

#### Permission Levels

| Permission | Description | Commands |
|------------|-------------|----------|
| `READ` | View files and directories | `LIST`, `PWD`, `CWD` |
| `WRITE` | Modify file attributes | `CHMOD`, `CHOWN` |
| `UPLOAD` | Upload files | `STOR`, `APPE`, `STOU` |
| `DOWNLOAD` | Download files | `RETR` |
| `DELETE` | Delete files and directories | `DELE`, `RMD` |
| `ADMIN` | Administrative operations | All commands |

#### User Types

**Regular Users**
- Full access to their home directory
- Configurable permissions and limits
- Session tracking and statistics

**Anonymous Users**
- Limited access to public directories
- No authentication required
- Restricted permissions and limits

**Guest Users**
- Temporary access with restrictions
- Limited session duration
- Minimal permissions

**Admin Users**
- Full system access
- User management capabilities
- Configuration modification rights

## 🌐 Virtual Host Management

### Creating Virtual Hosts

```bash
# Create basic virtual host
sudo ssftpd virtual add \
  --hostname ftp.example.com \
  --root /var/ftp/example.com \
  --ssl-cert /etc/ssftpd/ssl/example.com.crt \
  --ssl-key /etc/ssftpd/ssl/example.com.key

# Create virtual host with custom settings
sudo ssftpd virtual add \
  --hostname secure.example.com \
  --root /var/ftp/secure \
  --port 2121 \
  --ssl-port 9990 \
  --require-ssl \
  --max-connections 50 \
  --allowed-users admin,user1
```

### Managing Virtual Hosts

```bash
# List all virtual hosts
ssftpd virtual list

# Get virtual host details
ssftpd virtual info --hostname ftp.example.com

# Enable virtual host
sudo ssftpd virtual enable --hostname ftp.example.com

# Disable virtual host
sudo ssftpd virtual disable --hostname ftp.example.com

# Remove virtual host
sudo ssftpd virtual remove --hostname ftp.example.com
```

### Virtual Host Configuration

```bash
# Update virtual host settings
sudo ssftpd virtual modify \
  --hostname ftp.example.com \
  --max-connections 100 \
  --ssl-cert /etc/ssftpd/ssl/new-cert.crt \
  --ssl-key /etc/ssftpd/ssl/new-key.key

# Set default virtual host
sudo ssftpd virtual set-default --hostname ftp.example.com

# Test virtual host configuration
ssftpd virtual test --hostname ftp.example.com
```

## 🔒 SSL Certificate Management

### Generating Certificates

```bash
# Generate self-signed certificate
sudo ssftpd ssl generate \
  --hostname ftp.example.com \
  --country US \
  --state California \
  --city "San Francisco" \
  --organization "Example Corp" \
  --email admin@example.com \
  --days 365

# Generate certificate with custom settings
sudo ssftpd ssl generate \
  --hostname ftp.example.com \
  --key-size 4096 \
  --signature-algorithm sha256 \
  --days 730
```

### Installing Certificates

```bash
# Install existing certificate
sudo ssftpd ssl install \
  --hostname ftp.example.com \
  --cert /path/to/certificate.crt \
  --key /path/to/private.key

# Install with CA certificate
sudo ssftpd ssl install \
  --hostname ftp.example.com \
  --cert /path/to/certificate.crt \
  --key /path/to/private.key \
  --ca /path/to/ca.crt

# Install Let's Encrypt certificate
sudo ssftpd ssl install \
  --hostname ftp.example.com \
  --cert /etc/letsencrypt/live/ftp.example.com/fullchain.pem \
  --key /etc/letsencrypt/live/ftp.example.com/privkey.pem
```

### Managing Certificates

```bash
# List all certificates
ssftpd ssl list

# Check certificate status
ssftpd ssl status --hostname ftp.example.com

# Renew certificate
sudo ssftpd ssl renew --hostname ftp.example.com

# Remove certificate
sudo ssftpd ssl remove --hostname ftp.example.com

# Validate certificate
ssftpd ssl validate --hostname ftp.example.com
```

## 📊 Monitoring and Statistics

### Server Status

```bash
# Get server status
ssftpd status

# Get detailed status
ssftpd status --detailed

# Get status in JSON format
ssftpd status --json

# Get status for specific virtual host
ssftpd status --hostname ftp.example.com
```

### Connection Information

```bash
# List active connections
ssftpd connections

# Get connection details
ssftpd connections --detailed

# Filter connections by user
ssftpd connections --user john

# Filter connections by IP
ssftpd connections --ip 192.168.1.100
```

### Transfer Statistics

```bash
# Get transfer statistics
ssftpd transfers

# Get user transfer stats
ssftpd transfers --user john

# Get virtual host transfer stats
ssftpd transfers --hostname ftp.example.com

# Get transfer stats for time period
ssftpd transfers --since "2024-01-01" --until "2024-01-31"
```

### Performance Metrics

```bash
# Get performance metrics
ssftpd metrics

# Get system resource usage
ssftpd metrics --system

# Get network statistics
ssftpd metrics --network

# Get disk usage statistics
ssftpd metrics --disk
```

## 📝 Logging and Troubleshooting

### Viewing Logs

```bash
# View main server log
sudo tail -f /var/log/ssftpd/ssftpd.log

# View access log
sudo tail -f /var/log/ssftpd/access.log

# View error log
sudo tail -f /var/log/ssftpd/error.log

# View audit log
sudo tail -f /var/log/ssftpd/audit.log

# Search logs for specific user
sudo grep "john" /var/log/ssftpd/ssftpd.log

# Search logs for specific IP
sudo grep "192.168.1.100" /var/log/ssftpd/access.log
```

### Log Analysis

```bash
# Get log summary
ssftpd logs summary

# Get error count
ssftpd logs errors --count

# Get access patterns
ssftpd logs access --pattern

# Get user activity
ssftpd logs user --username john

# Export logs
ssftpd logs export --format csv --output /tmp/logs.csv
```

### Common Issues and Solutions

#### Connection Problems

```bash
# Check if server is running
ssftpd status

# Check if port is open
sudo netstat -tlnp | grep :21

# Check firewall rules
sudo ufw status
sudo firewall-cmd --list-all

# Test local connection
ftp localhost
```

#### Authentication Issues

```bash
# Check user configuration
ssftpd user info --username john

# Verify password
sudo ssftpd user password --username john --password newpass

# Check user permissions
ssftpd user permissions --username john

# Test user login
ssftpd user test --username john
```

#### SSL/TLS Issues

```bash
# Check certificate validity
ssftpd ssl status --hostname ftp.example.com

# Validate certificate
ssftpd ssl validate --hostname ftp.example.com

# Check SSL configuration
ssftpd ssl config --hostname ftp.example.com

# Test SSL connection
openssl s_client -connect ftp.example.com:990
```

## 🔧 Advanced Features

### Custom Commands

```bash
# Enable custom commands
ssftpd commands enable --command version

# Disable custom commands
ssftpd commands disable --command help

# List available commands
ssftpd commands list

# Test custom command
ssftpd commands test --command version
```

### File Transfer Optimization

```bash
# Enable sendfile optimization
ssftpd transfer optimize --sendfile

# Enable memory mapping
ssftpd transfer optimize --mmap

# Set buffer sizes
ssftpd transfer buffer --read 16384 --write 16384

# Enable compression
ssftpd transfer compression --enable --level 6
```

### Rate Limiting

```bash
# Set global rate limits
sudo ssftpd rate-limit set \
  --connections-per-minute 100 \
  --transfers-per-minute 50 \
  --bytes-per-minute 100MB

# Set per-user rate limits
sudo ssftpd rate-limit user \
  --username john \
  --connections-per-minute 10 \
  --transfer-rate 5MB/s

# Set per-IP rate limits
sudo ssftpd rate-limit ip \
  --ip 192.168.1.100 \
  --connections-per-minute 5 \
  --transfer-rate 2MB/s
```

## 📱 Web Interface

### Accessing the Web Interface

```bash
# Enable web interface
sudo ssftpd web enable --port 8080

# Access web interface
# Open browser to: http://localhost:8080

# Set web interface authentication
sudo ssftpd web auth \
  --username admin \
  --password adminpass

# Configure web interface
sudo ssftpd web config \
  --theme dark \
  --language en \
  --timezone UTC
```

### Web Interface Features

- **Server Status**: Real-time server information
- **User Management**: Add, modify, and remove users
- **Virtual Host Management**: Configure virtual hosts
- **SSL Certificate Management**: Manage SSL certificates
- **Monitoring**: View statistics and performance metrics
- **Logs**: Browse and search log files
- **File Browser**: Browse FTP directories

## 🔄 Backup and Recovery

### Configuration Backup

```bash
# Backup all configurations
sudo ssftpd backup create --output /backup/ssftpd-$(date +%Y%m%d).tar.gz

# Backup specific configuration
sudo ssftpd backup config --output /backup/config-$(date +%Y%m%d).tar.gz

# Backup user configurations
sudo ssftpd backup users --output /backup/users-$(date +%Y%m%d).tar.gz

# Backup SSL certificates
sudo ssftpd backup ssl --output /backup/ssl-$(date +%Y%m%d).tar.gz
```

### Configuration Restoration

```bash
# Restore from backup
sudo ssftpd backup restore --file /backup/ssftpd-20240115.tar.gz

# Restore specific configuration
sudo ssftpd backup restore-config --file /backup/config-20240115.tar.gz

# Restore users
sudo ssftpd backup restore-users --file /backup/users-20240115.tar.gz

# Restore SSL certificates
sudo ssftpd backup restore-ssl --file /backup/ssl-20240115.tar.gz
```

## 📚 Command Reference

### Server Management Commands

| Command | Description | Options |
|---------|-------------|---------|
| `ssftpd start` | Start the server | `--config`, `--daemon`, `--foreground` |
| `ssftpd stop` | Stop the server | `--force`, `--timeout` |
| `ssftpd restart` | Restart the server | `--config`, `--daemon` |
| `ssftpd reload` | Reload configuration | `--config` |
| `ssftpd status` | Show server status | `--detailed`, `--json` |

### User Management Commands

| Command | Description | Options |
|---------|-------------|---------|
| `ssftpd user add` | Add new user | `--username`, `--password`, `--home`, `--permissions` |
| `ssftpd user modify` | Modify user | `--username`, `--permissions`, `--limits` |
| `ssftpd user remove` | Remove user | `--username`, `--force` |
| `ssftpd user list` | List users | `--detailed`, `--json` |
| `ssftpd user info` | Show user info | `--username` |

### Virtual Host Commands

| Command | Description | Options |
|---------|-------------|---------|
| `ssftpd virtual add` | Add virtual host | `--hostname`, `--root`, `--ssl-cert`, `--ssl-key` |
| `ssftpd virtual modify` | Modify virtual host | `--hostname`, `--settings` |
| `ssftpd virtual remove` | Remove virtual host | `--hostname`, `--force` |
| `ssftpd virtual list` | List virtual hosts | `--detailed`, `--json` |
| `ssftpd virtual info` | Show virtual host info | `--hostname` |

### SSL Commands

| Command | Description | Options |
|---------|-------------|---------|
| `ssftpd ssl generate` | Generate certificate | `--hostname`, `--country`, `--state`, `--city` |
| `ssftpd ssl install` | Install certificate | `--hostname`, `--cert`, `--key`, `--ca` |
| `ssftpd ssl renew` | Renew certificate | `--hostname` |
| `ssftpd ssl remove` | Remove certificate | `--hostname` |
| `ssftpd ssl status` | Show certificate status | `--hostname` |

## 🚨 Troubleshooting Guide

### Common Error Messages

| Error | Cause | Solution |
|-------|-------|----------|
| `Connection refused` | Server not running | Start server with `ssftpd start` |
| `Authentication failed` | Invalid credentials | Check username/password |
| `Permission denied` | Insufficient permissions | Check user permissions |
| `SSL handshake failed` | Certificate issues | Validate SSL configuration |
| `Port already in use` | Port conflict | Change port or stop conflicting service |

### Diagnostic Commands

```bash
# Test server configuration
ssftpd --test-config --config /etc/ssftpd/ssftpd.conf

# Test SSL configuration
ssftpd --test-ssl --config /etc/ssftpd/ssftpd.conf

# Validate user configurations
ssftpd-ctl validate-users --config /etc/ssftpd/ssftpd.conf

# Check file permissions
ssftpd-ctl check-permissions --config /etc/ssftpd/ssftpd.conf

# Test network connectivity
ssftpd-ctl test-network --host localhost --port 21
```

### Performance Tuning

```bash
# Monitor performance
ssftpd metrics --real-time

# Analyze bottlenecks
ssftpd analyze --performance

# Optimize settings
ssftpd optimize --auto

# Generate performance report
ssftpd report --performance --output /tmp/performance.pdf
```

## 📚 Next Steps

After mastering the basic usage:

1. **Advanced Configuration**: See [Configuration Guide](../configuration/README.md)
2. **Security Hardening**: See [Security Guide](security.md)
3. **Performance Tuning**: See [Performance Guide](performance.md)
4. **Integration**: See [Integration Guide](integration.md)
5. **Development**: See [Development Guide](../development/README.md)

---

**Need help?** Check our [Troubleshooting Guide](troubleshooting.md) or open an [issue on GitHub](https://github.com/ssftpd/ssftpd/issues).
