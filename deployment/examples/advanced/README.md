# Advanced Setup Examples

This directory contains advanced deployment examples for enterprise and multi-instance scenarios.

## Multi-Instance Deployment

### Overview

Run multiple TFTP instances on different IP addresses and ports for different purposes:

- **Firmware Server**: 192.168.1.100:69 - Device firmware distribution
- **PXE Server**: 192.168.1.101:69 - Network boot services
- **Secure Server**: 10.0.0.100:6969 - Restricted access configuration

### 1. Create Instance Directories

```bash
# Create TFTP root directories
sudo mkdir -p /var/tftp/{firmware,pxe,secure}
sudo mkdir -p /var/log/simple-tftpd/{firmware,pxe,secure}

# Set permissions
sudo chown -R tftp:tftp /var/tftp /var/log/simple-tftpd
sudo chmod 755 /var/tftp /var/log/simple-tftpd
```

### 2. Configure Instances

```bash
# Create configuration directory
sudo mkdir -p /etc/simple-tftpd/instances

# Copy instance configurations
sudo cp deployment/configs/instances/*.conf /etc/simple-tftpd/instances/
```

### 3. Enable Multi-Instance Services

```bash
# Enable the target (manages all instances)
sudo systemctl enable simple-tftpd.target

# Start all instances
sudo systemctl start simple-tftpd.target

# Check status of all instances
sudo systemctl status simple-tftpd.target
```

### 4. Individual Instance Management

```bash
# Start specific instance
sudo systemctl start simple-tftpd@firmware
sudo systemctl start simple-tftpd@pxe
sudo systemctl start simple-tftpd@secure

# Check specific instance status
sudo systemctl status simple-tftpd@firmware

# View instance logs
sudo journalctl -u simple-tftpd@firmware -f
```

## Enterprise Configuration

### High-Availability Setup

#### Load Balancer Configuration (HAProxy)

```haproxy
# /etc/haproxy/haproxy.cfg
global
    log /dev/log local0
    chroot /var/lib/haproxy
    stats socket /run/haproxy/admin.sock mode 660 level admin
    stats timeout 30s
    user haproxy
    group haproxy
    daemon

defaults
    log global
    mode tcp
    option tcplog
    option dontlognull
    timeout connect 5000
    timeout client 50000
    timeout server 50000

frontend tftp_frontend
    bind *:69
    mode tcp
    default_backend tftp_backend

backend tftp_backend
    mode tcp
    balance roundrobin
    server tftp1 192.168.1.100:69 check
    server tftp2 192.168.1.101:69 check
    server tftp3 192.168.1.102:69 check
```

#### Keepalived Configuration

```bash
# /etc/keepalived/keepalived.conf
vrrp_instance VI_1 {
    state MASTER
    interface eth0
    virtual_router_id 51
    priority 100
    advert_int 1
    authentication {
        auth_type PASS
        auth_pass 1111
    }
    virtual_ipaddress {
        192.168.1.99
    }
}
```

### Network Segmentation

#### Firewall Rules (iptables)

```bash
# Allow TFTP traffic on specific interfaces
iptables -A INPUT -i eth0 -p udp --dport 69 -j ACCEPT
iptables -A INPUT -i eth1 -p udp --dport 69 -j ACCEPT

# Block TFTP on other interfaces
iptables -A INPUT -p udp --dport 69 -j DROP

# Allow specific source networks
iptables -A INPUT -s 192.168.1.0/24 -p udp --dport 69 -j ACCEPT
iptables -A INPUT -s 10.0.0.0/24 -p udp --dport 6969 -j ACCEPT
```

#### Firewall Rules (firewalld)

```bash
# Create custom zone for TFTP
sudo firewall-cmd --permanent --new-zone=tftp

# Add source networks
sudo firewall-cmd --permanent --zone=tftp --add-source=192.168.1.0/24
sudo firewall-cmd --permanent --zone=tftp --add-source=10.0.0.0/24

# Add TFTP service
sudo firewall-cmd --permanent --zone=tftp --add-service=tftp

# Reload firewall
sudo firewall-cmd --reload
```

## Monitoring and Logging

### Centralized Logging (rsyslog)

```bash
# /etc/rsyslog.d/simple-tftpd.conf
# Forward TFTP logs to central server
if $programname == 'simple-tftpd' then @192.168.1.200:514
if $programname == 'simple-tftpd' then stop

# Local logging
if $programname == 'simple-tftpd' then /var/log/simple-tftpd/combined.log
```

### Monitoring with Prometheus

#### Exporter Configuration

```yaml
# /etc/simple-tftpd/prometheus.yml
global:
  scrape_interval: 15s

scrape_configs:
  - job_name: 'simple-tftpd'
    static_configs:
      - targets: ['localhost:9090']
    metrics_path: /metrics
    scrape_interval: 5s
```

#### Grafana Dashboard

```json
{
  "dashboard": {
    "title": "Simple TFTP Daemon Metrics",
    "panels": [
      {
        "title": "Active Connections",
        "type": "stat",
        "targets": [
          {
            "expr": "simple_tftpd_active_connections",
            "legendFormat": "Connections"
          }
        ]
      },
      {
        "title": "Transfer Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(simple_tftpd_bytes_transferred[5m])",
            "legendFormat": "Bytes/sec"
          }
        ]
      }
    ]
  }
}
```

## Security Hardening

### SELinux Configuration

```bash
# Create SELinux policy
cat > simple-tftpd.te << EOF
module simple-tftpd 1.0;

require {
    type tftp_exec_t;
    type tftp_data_t;
    type tftp_log_t;
    type tftp_var_lib_t;
}

# Allow TFTP daemon to read/write TFTP data
allow tftp_exec_t tftp_data_t:dir { read write search };
allow tftp_exec_t tftp_data_t:file { read write };

# Allow TFTP daemon to write logs
allow tftp_exec_t tftp_log_t:file { write append };

# Allow TFTP daemon to access var/lib
allow tftp_exec_t tftp_var_lib_t:dir { read write search };
EOF

# Compile and install policy
make -f /usr/share/selinux/devel/Makefile simple-tftpd.pp
sudo semodule -i simple-tftpd.pp
```

### AppArmor Profile

```bash
# /etc/apparmor.d/usr.bin.simple-tftpd
#include <tunables/global>

/usr/bin/simple-tftpd {
  #include <abstractions/base>
  #include <abstractions/nameservice>
  #include <abstractions/user-tmp>

  # TFTP root directory
  /var/tftp/** r,
  /var/tftp/** w,

  # Configuration files
  /etc/simple-tftpd/** r,
  /etc/simple-tftpd/instances/** r,

  # Log files
  /var/log/simple-tftpd/** w,
  /var/log/simple-tftpd/instances/** w,

  # Deny access to sensitive files
  deny /etc/passwd r,
  deny /etc/shadow r,
  deny /root/** r,
  deny /home/** r,
}
```

## Performance Tuning

### System Tuning

```bash
# /etc/sysctl.d/99-tftp-tuning.conf
# Increase UDP buffer sizes
net.core.rmem_max = 16777216
net.core.wmem_max = 16777216
net.core.rmem_default = 1048576
net.core.wmem_default = 1048576

# Increase file descriptor limits
fs.file-max = 1000000

# Optimize network settings
net.core.netdev_max_backlog = 5000
net.core.somaxconn = 65535
```

### TFTP-Specific Tuning

```json
{
    "performance": {
        "block_size": 1024,
        "timeout": 10,
        "window_size": 4,
        "max_connections": 500,
        "connection_timeout": 600
    }
}
```

## Backup and Recovery

### Configuration Backup

```bash
#!/bin/bash
# /usr/local/bin/backup-tftp-config.sh

BACKUP_DIR="/backup/simple-tftpd"
DATE=$(date +%Y%m%d_%H%M%S)

# Create backup directory
mkdir -p $BACKUP_DIR

# Backup configurations
tar -czf $BACKUP_DIR/config_$DATE.tar.gz \
    /etc/simple-tftpd \
    /var/tftp \
    /var/log/simple-tftpd

# Keep only last 7 days of backups
find $BACKUP_DIR -name "config_*.tar.gz" -mtime +7 -delete

echo "Backup completed: $BACKUP_DIR/config_$DATE.tar.gz"
```

### Disaster Recovery

```bash
#!/bin/bash
# /usr/local/bin/restore-tftp-config.sh

BACKUP_FILE=$1
RESTORE_DIR="/tmp/tftp-restore"

if [ -z "$BACKUP_FILE" ]; then
    echo "Usage: $0 <backup-file>"
    exit 1
fi

# Extract backup
mkdir -p $RESTORE_DIR
tar -xzf $BACKUP_FILE -C $RESTORE_DIR

# Stop services
systemctl stop simple-tftpd.target

# Restore configurations
cp -r $RESTORE_DIR/etc/simple-tftpd/* /etc/simple-tftpd/
cp -r $RESTORE_DIR/var/tftp/* /var/tftp/

# Set permissions
chown -R tftp:tftp /etc/simple-tftpd /var/tftp

# Start services
systemctl start simple-tftpd.target

# Cleanup
rm -rf $RESTORE_DIR

echo "Restore completed"
```

## Troubleshooting Advanced Deployments

### Multi-Instance Issues

```bash
# Check all instance statuses
sudo systemctl status simple-tftpd.target

# View logs for specific instance
sudo journalctl -u simple-tftpd@firmware -f

# Test instance configuration
sudo simple-tftpd test --config /etc/simple-tftpd/instances/firmware.conf

# Check network binding
sudo netstat -tulpn | grep simple-tftpd
```

### Performance Issues

```bash
# Check system resources
htop
iotop
nethogs

# Monitor network performance
iperf3 -s
iperf3 -c <client-ip>

# Check TFTP performance
sudo simple-tftpd stats
sudo simple-tftpd connections
```

### Security Issues

```bash
# Check SELinux status
sudo semanage port -l | grep tftp
sudo ausearch -m AVC -ts recent

# Check AppArmor status
sudo aa-status
sudo aa-logprof

# Check firewall rules
sudo iptables -L -n | grep 69
sudo firewall-cmd --list-all
```

## Next Steps

After setting up advanced deployment:

1. **[Simple Setup](../simple/README.md)** - Review basic configuration
2. **[Configuration Reference](../../../docs/configuration/README.md)** - Understand all options
3. **[User Guide](../../../docs/user-guide/README.md)** - Learn advanced usage
4. **[Development Guide](../../../docs/development/README.md)** - Contribute improvements
