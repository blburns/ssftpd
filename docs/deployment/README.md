# Deployment Guide

This guide covers deploying ssftpd in various environments, from development to production.

## 🐳 Docker Deployment (Recommended)

Docker is the recommended deployment method for ssftpd, providing:

- **Zero dependencies** - No need to install build tools or dependencies
- **Cross-platform** - Works consistently across Linux, macOS, and Windows
- **Production-ready** - Optimized runtime images with security hardening
- **Easy scaling** - Simple horizontal scaling with Docker Compose
- **Isolation** - Complete environment isolation and dependency management

### Quick Start

```bash
# Clone and navigate to Docker examples
git clone <repository-url>
cd simple-sftpd/deployment/examples/docker

# Start the service
docker-compose up -d

# Test the service
nc -z localhost 21
```

### Docker Features

- **Multi-stage builds** for different Linux distributions
- **Multi-architecture support** (x86_64, ARM64, ARMv7)
- **Development environment** with debugging tools
- **Production runtime** with minimal footprint
- **Health checks** and monitoring
- **Volume mounts** for configuration and data

### Docker Commands

```bash
# Development environment
docker-compose --profile dev up -d

# Production deployment
docker-compose --profile runtime up -d

# Build for all platforms
./scripts/build-docker.sh -d all

# Deploy with custom directories
./scripts/deploy-docker.sh -p runtime -c ./config -l ./logs -d ./data
```

For detailed Docker deployment, see [Docker Deployment Guide](docker.md).

## Traditional Deployment

### System Requirements

- **Operating System**: Linux, macOS, or Windows
- **Memory**: 256MB minimum, 512MB recommended
- **Storage**: 100MB for application, additional space for FTP data
- **Network**: Ports 21 (FTP), 990 (FTPS), and passive mode range (1024-65535)

### Installation Methods

#### From Source

```bash
# Install dependencies
make install-dev

# Build and install
make build
make install
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

1. **Copy example configuration:**
   ```bash
   sudo cp /etc/ssftpd/ssftpd.conf.example /etc/ssftpd/ssftpd.conf
   ```

2. **Edit configuration:**
   ```bash
   sudo nano /etc/ssftpd/ssftpd.conf
   ```

3. **Validate configuration:**
   ```bash
   ssftpd --test-config
   ```

### Service Management

#### Linux (systemd)

```bash
# Install service
sudo systemctl enable ssftpd
sudo systemctl start ssftpd

# Check status
sudo systemctl status ssftpd

# View logs
sudo journalctl -u ssftpd -f
```

#### macOS (launchd)

```bash
# Install service
sudo cp scripts/com.simpledaemons.ssftpd.plist /Library/LaunchDaemons/
sudo launchctl load /Library/LaunchDaemons/com.simpledaemons.ssftpd.plist

# Check status
sudo launchctl list | grep ssftpd
```

#### Windows (Service)

```cmd
# Install service
sc create ssftpd binPath="C:\Program Files\ssftpd\ssftpd.exe" start=auto
sc start ssftpd

# Check status
sc query ssftpd
```

## Production Deployment

### Security Considerations

1. **Run as non-root user**
2. **Use SSL/TLS for secure transfers**
3. **Implement proper firewall rules**
4. **Regular security updates**
5. **Monitor access logs**
6. **Use strong authentication**

### Performance Tuning

#### System Optimization

```bash
# Increase file descriptor limits
echo '* soft nofile 65536' >> /etc/security/limits.conf
echo '* hard nofile 65536' >> /etc/security/limits.conf

# Increase TCP buffer sizes
echo 'net.core.rmem_max=26214400' >> /etc/sysctl.conf
echo 'net.core.wmem_max=26214400' >> /etc/sysctl.conf
sysctl -p
```

#### Application Configuration

```ini
[connection]
max_connections = 1000
max_connections_per_ip = 10

[performance]
sendfile_enabled = true
mmap_enabled = true
```

### Monitoring

#### Health Checks

```bash
# Check service status
systemctl status ssftpd

# Test FTP connectivity
nc -z localhost 21

# Check SSL/TLS
openssl s_client -connect localhost:990 -starttls ftp
```

#### Log Monitoring

```bash
# Follow logs in real-time
tail -f /var/log/ssftpd/ssftpd.log

# Search for errors
grep ERROR /var/log/ssftpd/ssftpd.log

# Monitor access logs
tail -f /var/log/ssftpd/access.log
```

### Backup and Recovery

#### Backup Script

```bash
#!/bin/bash
# backup-ssftpd.sh

BACKUP_DIR="./backup/$(date +%Y%m%d_%H%M%S)"
mkdir -p "$BACKUP_DIR"

# Backup configuration
cp -r /etc/ssftpd "$BACKUP_DIR/"

# Backup FTP data
cp -r /var/ftp "$BACKUP_DIR/"

# Backup logs
cp -r /var/log/ssftpd "$BACKUP_DIR/"

echo "Backup completed: $BACKUP_DIR"
```

#### Restore Script

```bash
#!/bin/bash
# restore-ssftpd.sh

BACKUP_DIR="$1"
if [ -z "$BACKUP_DIR" ]; then
    echo "Usage: $0 <backup_directory>"
    exit 1
fi

# Stop service
systemctl stop ssftpd

# Restore files
cp -r "$BACKUP_DIR/ssftpd" /etc/
cp -r "$BACKUP_DIR/ftp" /var/
cp -r "$BACKUP_DIR/ssftpd" /var/log/

# Start service
systemctl start ssftpd

echo "Restore completed from: $BACKUP_DIR"
```

## Scaling

### Horizontal Scaling

#### Load Balancer Configuration

```yaml
# haproxy.cfg
global
    daemon

defaults
    mode tcp
    timeout connect 5000ms
    timeout client 50000ms
    timeout server 50000ms

frontend ftp_frontend
    bind *:21
    default_backend ftp_servers

backend ftp_servers
    balance roundrobin
    server ftp1 10.0.1.10:21 check
    server ftp2 10.0.1.11:21 check
    server ftp3 10.0.1.12:21 check
```

#### Docker Swarm

```yaml
version: '3.8'
services:
  ssftpd:
    image: ssftpd:latest
    deploy:
      replicas: 3
      endpoint_mode: dnsrr
    ports:
      - "21:21"
      - "990:990"
    volumes:
      - ftp_data:/var/ftp
      - ftp_config:/etc/ssftpd:ro

volumes:
  ftp_data:
  ftp_config:
```

### Vertical Scaling

#### Resource Limits

```yaml
# docker-compose.yml
services:
  ssftpd:
    deploy:
      resources:
        limits:
          memory: 1G
          cpus: '2.0'
        reservations:
          memory: 512M
          cpus: '1.0'
```

## Troubleshooting

### Common Issues

1. **Port conflicts**
   ```bash
   # Check port usage
   sudo lsof -i :21
   sudo lsof -i :990
   ```

2. **Permission issues**
   ```bash
   # Fix directory permissions
   sudo chown -R ssftpd:ssftpd /var/ftp
   sudo chmod 755 /var/ftp
   ```

3. **Configuration errors**
   ```bash
   # Validate configuration
   ssftpd --test-config
   ```

### Debug Mode

```bash
# Run with debug logging
ssftpd --verbose --foreground

# Check configuration
ssftpd --test-config --config /etc/ssftpd/ssftpd.conf
```

## Support

- **Documentation**: [Configuration Guide](../configuration/README.md)
- **Examples**: [Deployment Examples](../examples/README.md)
- **Troubleshooting**: [Troubleshooting Guide](../troubleshooting/README.md)
- **Community Support**: [GitHub Issues](https://github.com/your-org/simple-sftpd/issues)
