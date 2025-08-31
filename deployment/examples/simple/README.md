# Simple Setup Examples

This directory contains simple, single-instance deployment examples for the Simple TFTP Daemon.

## Quick Start

### 1. Install the Package

```bash
# Ubuntu/Debian
sudo apt install simple-tftpd

# CentOS/RHEL
sudo yum install simple-tftpd

# From source
make install
```

### 2. Create Basic Configuration

```bash
# Create configuration directory
sudo mkdir -p /etc/simple-tftpd

# Copy example configuration
sudo cp /usr/share/simple-tftpd/simple-tftpd.conf.example /etc/simple-tftpd/simple-tftpd.conf

# Create TFTP root directory
sudo mkdir -p /var/tftp
sudo chown tftp:tftp /var/tftp
```

### 3. Start the Service

```bash
# Enable and start service
sudo systemctl enable simple-tftpd
sudo systemctl start simple-tftpd

# Check status
sudo systemctl status simple-tftpd
```

### 4. Test the Service

```bash
# Test file download
tftp localhost 69
tftp> get testfile.txt
tftp> quit
```

## Basic Configuration

### Single Instance Configuration

```json
{
    "network": {
        "listen_address": "0.0.0.0",
        "listen_port": 69
    },
    "filesystem": {
        "root_directory": "/var/tftp"
    },
    "security": {
        "read_enabled": true,
        "write_enabled": false
    }
}
```

### Directory Structure

```
/var/tftp/
├── firmware/
├── configs/
└── images/
```

## Service Management

### Start/Stop Service

```bash
# Start service
sudo systemctl start simple-tftpd

# Stop service
sudo systemctl stop simple-tftpd

# Restart service
sudo systemctl restart simple-tftpd

# Reload configuration
sudo systemctl reload simple-tftpd
```

### Check Service Status

```bash
# Check if running
sudo systemctl is-active simple-tftpd

# View logs
sudo journalctl -u simple-tftpd -f

# Check configuration
sudo simple-tftpd test --config /etc/simple-tftpd/simple-tftpd.conf
```

## Troubleshooting

### Common Issues

1. **Port already in use**
   ```bash
   sudo netstat -tulpn | grep :69
   sudo systemctl stop tftpd  # Stop system TFTP service
   ```

2. **Permission denied**
   ```bash
   sudo chown -R tftp:tftp /var/tftp
   sudo chmod 755 /var/tftp
   ```

3. **Service won't start**
   ```bash
   sudo journalctl -u simple-tftpd -n 50
   sudo simple-tftpd test --config /etc/simple-tftpd/simple-tftpd.conf
   ```

### Getting Help

- Check service logs: `sudo journalctl -u simple-tftpd`
- Test configuration: `sudo simple-tftpd test`
- Check file permissions: `ls -la /var/tftp`
- Verify network: `sudo netstat -tulpn | grep :69`
