# Deployment Guide

This directory contains everything needed to deploy the Simple TFTP Daemon in production environments, from simple single-instance setups to complex multi-instance enterprise deployments.

## Directory Structure

```
deployment/
├── README.md                 # This file - deployment overview
├── systemd/                  # systemd service files
│   ├── simple-tftpd.service # Main service file
│   ├── simple-tftpd@.service # Template for multiple instances
│   └── simple-tftpd.target  # Target for managing instances
├── init.d/                   # Traditional init.d scripts
│   └── simple-tftpd         # init.d service script
├── configs/                  # Configuration files
│   ├── simple-tftpd.conf    # Main configuration
│   └── instances/           # Instance-specific configs
│       ├── firmware.conf    # Firmware distribution
│       ├── pxe.conf         # PXE boot services
│       └── secure.conf      # Secure restricted access
├── examples/                 # Deployment examples
│   ├── simple/              # Basic single-instance setup
│   └── advanced/            # Multi-instance enterprise setup
├── scripts/                  # Deployment scripts
│   └── install.sh           # Automated installation script
└── templates/                # Configuration templates
```

## Quick Start

### Automated Installation

```bash
# Clone the repository
git clone https://github.com/your-username/simple-tftpd.git
cd simple-tftpd

# Run the installation script
sudo ./deployment/scripts/install.sh
```

### Manual Installation

1. **Install the binary**: `make install`
2. **Copy service files**: Copy from `deployment/systemd/` or `deployment/init.d/`
3. **Copy configurations**: Copy from `deployment/configs/`
4. **Create directories**: Set up TFTP root and log directories
5. **Start services**: Enable and start the service

## Deployment Options

### 1. Simple Single-Instance

Perfect for basic TFTP needs:

- Single TFTP server on port 69
- Basic security and performance settings
- Easy to configure and maintain

**Files needed**:
- `deployment/systemd/simple-tftpd.service`
- `deployment/configs/simple-tftpd.conf`

**Setup**: See `examples/simple/README.md`

### 2. Multi-Instance Deployment

For enterprise environments with multiple TFTP services:

- Multiple instances on different IPs/ports
- Purpose-specific configurations (firmware, PXE, secure)
- Centralized management with systemd targets

**Files needed**:
- `deployment/systemd/simple-tftpd@.service`
- `deployment/systemd/simple-tftpd.target`
- `deployment/configs/instances/*.conf`

**Setup**: See `examples/advanced/README.md`

### 3. Enterprise Deployment

For large-scale production environments:

- High-availability with load balancers
- Network segmentation and security
- Monitoring and centralized logging
- Backup and disaster recovery

**Additional components**:
- HAProxy/Keepalived for load balancing
- Prometheus/Grafana for monitoring
- rsyslog for centralized logging
- SELinux/AppArmor for security

## Service Management

### systemd (Recommended)

```bash
# Single instance
sudo systemctl start simple-tftpd
sudo systemctl enable simple-tftpd
sudo systemctl status simple-tftpd

# Multiple instances
sudo systemctl start simple-tftpd.target
sudo systemctl start simple-tftpd@firmware
sudo systemctl start simple-tftpd@pxe
```

### init.d (Legacy)

```bash
# Service management
sudo /etc/init.d/simple-tftpd start
sudo /etc/init.d/simple-tftpd stop
sudo /etc/init.d/simple-tftpd restart
sudo /etc/init.d/simple-tftpd status
```

## Configuration Management

### Main Configuration

The main configuration file (`simple-tftpd.conf`) controls:

- Network binding (IP, port, IPv6)
- File system access (root directory, allowed paths)
- Security settings (read/write, file types, size limits)
- Performance tuning (block size, timeouts, connections)
- Logging configuration (level, files, rotation)

### Instance Configurations

Instance-specific configurations allow:

- Different IP addresses and ports
- Separate root directories
- Purpose-specific security policies
- Custom performance settings
- Individual logging

### Environment Variables

Override configuration values:

```bash
export SIMPLE_TFTPD_NETWORK_LISTEN_ADDRESS=192.168.1.100
export SIMPLE_TFTPD_NETWORK_LISTEN_PORT=6969
export SIMPLE_TFTPD_FILESYSTEM_ROOT_DIRECTORY=/var/tftp/custom
```

## Security Considerations

### Network Security

- Bind to specific interfaces, not 0.0.0.0
- Use firewall rules to restrict access
- Consider network segmentation for different instance types

### File System Security

- Restrict allowed directories
- Set appropriate file permissions
- Use allowed/blocked file extensions
- Enable overwrite protection

### Service Security

- Run as dedicated user (tftp)
- Use systemd security features
- Implement SELinux/AppArmor policies
- Regular security updates

## Performance Tuning

### System Level

- Increase UDP buffer sizes
- Optimize file descriptor limits
- Tune network parameters

### Application Level

- Adjust block sizes for network conditions
- Set appropriate timeouts
- Configure connection limits
- Monitor resource usage

## Monitoring and Logging

### Service Monitoring

- Check service status regularly
- Monitor log files for errors
- Track performance metrics
- Set up alerting for failures

### Centralized Logging

- Forward logs to central server
- Implement log rotation
- Monitor log file sizes
- Archive old logs

## Backup and Recovery

### Configuration Backup

- Backup configuration files
- Backup TFTP root directories
- Document customizations
- Test restore procedures

### Disaster Recovery

- Document recovery procedures
- Test backup restoration
- Maintain recovery documentation
- Regular recovery drills

## Troubleshooting

### Common Issues

1. **Service won't start**: Check configuration, permissions, and logs
2. **Permission denied**: Verify user/group ownership and file permissions
3. **Port conflicts**: Check for existing TFTP services
4. **Network issues**: Verify firewall rules and network configuration

### Debug Mode

```bash
# Enable debug logging
sudo simple-tftpd --verbose start

# Check service logs
sudo journalctl -u simple-tftpd -f

# Test configuration
sudo simple-tftpd test --config /etc/simple-tftpd/simple-tftpd.conf
```

## Support and Resources

### Documentation

- **[Installation Guide](../../docs/installation/README.md)** - Complete installation instructions
- **[User Guide](../../docs/user-guide/README.md)** - Usage and management
- **[Configuration Reference](../../docs/configuration/README.md)** - All configuration options
- **[Development Guide](../../docs/development/README.md)** - Contributing to the project

### Getting Help

- Check this deployment guide first
- Review example configurations
- Check service logs for errors
- Search existing GitHub issues
- Create new issue with detailed information

## Next Steps

After deployment:

1. **Test the service** with TFTP clients
2. **Configure monitoring** and alerting
3. **Set up backups** and recovery procedures
4. **Document customizations** and procedures
5. **Plan for scaling** as needs grow

## Contributing

We welcome contributions to improve deployment:

- Share deployment experiences
- Submit configuration examples
- Improve installation scripts
- Add new deployment scenarios
- Enhance security configurations

The deployment system is designed to be simple yet powerful, allowing administrators to start with basic configurations and scale to enterprise deployments as needed.
