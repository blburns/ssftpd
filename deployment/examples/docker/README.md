# Docker Deployment Guide

This guide covers deploying ssftpd using Docker containers.

## Quick Start

### 1. Clone the Repository

```bash
git clone <repository-url>
cd simple-sftpd
```

### 2. Create Configuration Directory

```bash
mkdir -p deployment/examples/docker/config
mkdir -p deployment/examples/docker/logs
mkdir -p deployment/examples/docker/data
cp config/examples/simple/ssftpd.conf.example deployment/examples/docker/config/ssftpd.conf
```

### 3. Build and Run

```bash
cd deployment/examples/docker
docker-compose up -d
```

### 4. Verify Deployment

```bash
# Check container status
docker-compose ps

# Check logs
docker-compose logs -f ssftpd

# Test FTP service
nc -z localhost 21
```

## Configuration

### Environment Variables

The following environment variables can be set in the docker-compose.yml:

- `FTP_ROOT_DIR`: FTP root directory (default: /var/ftp)
- `FTP_CONFIG_FILE`: Configuration file path (default: /etc/ssftpd/ssftpd.conf)
- `LOG_LEVEL`: Log level (default: INFO)

### Volume Mounts

- `./config:/etc/ssftpd:ro`: Configuration files (read-only)
- `./logs:/var/log/ssftpd`: Log files
- `./data:/var/ftp`: FTP data directory

## Production Deployment

### 1. Create Production Configuration

```bash
# Copy production config
cp config/examples/production/enterprise.conf deployment/examples/docker/config/ssftpd.conf

# Edit configuration
nano deployment/examples/docker/config/ssftpd.conf
```

### 2. Set Resource Limits

```yaml
services:
  ssftpd:
    # ... existing configuration ...
    deploy:
      resources:
        limits:
          memory: 512M
          cpus: '1.0'
        reservations:
          memory: 256M
          cpus: '0.5'
```

### 3. Enable Logging Driver

```yaml
services:
  ssftpd:
    # ... existing configuration ...
    logging:
      driver: "json-file"
      options:
        max-size: "10m"
        max-file: "3"
```

## Monitoring

### Health Checks

The container includes a health check that verifies FTP service availability:

```bash
# Check health status
docker inspect --format='{{.State.Health.Status}}' ssftpd
```

### Log Monitoring

```bash
# Follow logs in real-time
docker-compose logs -f ssftpd

# Search logs for errors
docker-compose logs ssftpd | grep ERROR

# Export logs
docker-compose logs ssftpd > ftp-logs.txt
```

## Troubleshooting

### Common Issues

1. **Port Already in Use**
   ```bash
   # Check what's using port 21
   sudo lsof -i :21

   # Stop conflicting service
   sudo systemctl stop vsftpd
   ```

2. **Permission Issues**
   ```bash
   # Fix log directory permissions
   sudo chown -R 1000:1000 deployment/examples/docker/logs
   sudo chown -R 1000:1000 deployment/examples/docker/data
   ```

3. **Configuration Errors**
   ```bash
   # Validate configuration
   docker-compose exec ssftpd ssftpd --test-config
   ```

### Debug Mode

```bash
# Run with debug logging
docker-compose run --rm ssftpd ssftpd --verbose --foreground
```

## Scaling

### Multiple Instances

```yaml
services:
  ssftpd:
    # ... existing configuration ...
    deploy:
      replicas: 3
      endpoint_mode: dnsrr
```

### Load Balancing

Use an external load balancer (HAProxy, nginx) to distribute FTP connections across multiple containers.

## Security Considerations

- The container runs as a non-root user (ssftpd)
- Configuration files are mounted as read-only
- FTP ports (21, 990) and passive mode range (1024-65535) are exposed
- Network isolation using custom bridge network
- SSL/TLS support for secure file transfers

## Backup and Recovery

### Backup Configuration

```bash
# Backup configuration
docker cp ssftpd:/etc/ssftpd/ssftpd.conf ./backup/

# Backup FTP data
docker cp ssftpd:/var/ftp ./backup/
```

### Restore Configuration

```bash
# Restore configuration
docker cp ./backup/ssftpd.conf ssftpd:/etc/ssftpd/
docker cp ./backup/ftp ssftpd:/var/

# Restart service
docker-compose restart ssftpd
```

## Performance Tuning

### Container Optimization

```yaml
services:
  ssftpd:
    # ... existing configuration ...
    ulimits:
      nofile:
        soft: 65536
        hard: 65536
    sysctls:
      - net.core.rmem_max=26214400
      - net.core.wmem_max=26214400
```

### Host System Tuning

```bash
# Increase TCP buffer sizes
echo 'net.core.rmem_max=26214400' >> /etc/sysctl.conf
echo 'net.core.wmem_max=26214400' >> /etc/sysctl.conf
sysctl -p
```

## FTP Client Testing

### Basic FTP Test

```bash
# Test FTP connection
ftp localhost 21

# Test FTPS connection (if SSL enabled)
lftp -e "set ssl:verify-certificate no; open -p 990 localhost; ls; quit"
```

### Passive Mode Testing

```bash
# Test passive mode with specific port range
ftp -p localhost 21
```
