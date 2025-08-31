#!/bin/bash
#
# Simple TFTP Daemon Installation Script
# This script installs and configures simple-tftpd for production use
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
TFTP_USER="tftp"
TFTP_GROUP="tftp"
TFTP_HOME="/var/tftp"
TFTP_CONFIG_DIR="/etc/simple-tftpd"
TFTP_LOG_DIR="/var/log/simple-tftpd"
TFTP_BIN="/usr/bin/simple-tftpd"

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if running as root
check_root() {
    if [[ $EUID -ne 0 ]]; then
        print_error "This script must be run as root"
        exit 1
    fi
}

# Function to detect OS
detect_os() {
    if [[ -f /etc/os-release ]]; then
        . /etc/os-release
        OS=$NAME
        VER=$VERSION_ID
    elif type lsb_release >/dev/null 2>&1; then
        OS=$(lsb_release -si)
        VER=$(lsb_release -sr)
    elif [[ -f /etc/lsb-release ]]; then
        . /etc/lsb-release
        OS=$DISTRIB_ID
        VER=$DISTRIB_RELEASE
    elif [[ -f /etc/debian_version ]]; then
        OS=Debian
        VER=$(cat /etc/debian_version)
    elif [[ -f /etc/SuSe-release ]]; then
        OS=SuSE
    elif [[ -f /etc/redhat-release ]]; then
        OS=RedHat
    else
        OS=$(uname -s)
        VER=$(uname -r)
    fi
    
    print_status "Detected OS: $OS $VER"
}

# Function to install dependencies
install_dependencies() {
    print_status "Installing dependencies..."
    
    case $OS in
        *Ubuntu*|*Debian*)
            apt-get update
            apt-get install -y build-essential cmake git pkg-config
            ;;
        *CentOS*|*RedHat*|*Fedora*)
            yum groupinstall -y "Development Tools"
            yum install -y cmake3 git
            ;;
        *)
            print_warning "Unknown OS, please install dependencies manually"
            ;;
    esac
}

# Function to create TFTP user
create_tftp_user() {
    print_status "Creating TFTP user and group..."
    
    # Check if user exists
    if ! id "$TFTP_USER" &>/dev/null; then
        useradd -r -s /bin/false -d $TFTP_HOME $TFTP_USER
        print_success "Created user $TFTP_USER"
    else
        print_status "User $TFTP_USER already exists"
    fi
    
    # Check if group exists
    if ! getent group "$TFTP_GROUP" &>/dev/null; then
        groupadd -r $TFTP_GROUP
        print_success "Created group $TFTP_GROUP"
    else
        print_status "Group $TFTP_GROUP already exists"
    fi
    
    # Add user to group if not already
    if ! groups $TFTP_USER | grep -q $TFTP_GROUP; then
        usermod -a -G $TFTP_GROUP $TFTP_USER
        print_success "Added $TFTP_USER to group $TFTP_GROUP"
    fi
}

# Function to create directories
create_directories() {
    print_status "Creating directories..."
    
    # Create TFTP directories
    mkdir -p $TFTP_HOME
    mkdir -p $TFTP_CONFIG_DIR
    mkdir -p $TFTP_LOG_DIR
    
    # Set ownership
    chown $TFTP_USER:$TFTP_GROUP $TFTP_HOME
    chown $TFTP_USER:$TFTP_GROUP $TFTP_CONFIG_DIR
    chown $TFTP_USER:$TFTP_GROUP $TFTP_LOG_DIR
    
    # Set permissions
    chmod 755 $TFTP_HOME
    chmod 755 $TFTP_CONFIG_DIR
    chmod 755 $TFTP_LOG_DIR
    
    print_success "Created and configured directories"
}

# Function to install systemd services
install_systemd_services() {
    print_status "Installing systemd services..."
    
    # Copy service files
    cp deployment/systemd/simple-tftpd.service /etc/systemd/system/
    cp deployment/systemd/simple-tftpd@.service /etc/systemd/system/
    cp deployment/systemd/simple-tftpd.target /etc/systemd/system/
    
    # Reload systemd
    systemctl daemon-reload
    
    print_success "Installed systemd services"
}

# Function to install init.d scripts
install_initd_scripts() {
    print_status "Installing init.d scripts..."
    
    # Copy init.d script
    cp deployment/init.d/simple-tftpd /etc/init.d/
    chmod +x /etc/init.d/simple-tftpd
    
    # Enable service
    if command -v chkconfig &> /dev/null; then
        chkconfig --add simple-tftpd
        chkconfig --level 2345 simple-tftpd on
    elif command -v systemctl &> /dev/null; then
        systemctl enable simple-tftpd
    fi
    
    print_success "Installed init.d scripts"
}

# Function to install configurations
install_configurations() {
    print_status "Installing configurations..."
    
    # Copy main configuration
    cp deployment/configs/simple-tftpd.conf $TFTP_CONFIG_DIR/
    
    # Create instances directory
    mkdir -p $TFTP_CONFIG_DIR/instances
    
    # Copy instance configurations
    cp deployment/configs/instances/*.conf $TFTP_CONFIG_DIR/instances/
    
    # Set ownership
    chown -R $TFTP_USER:$TFTP_GROUP $TFTP_CONFIG_DIR
    
    print_success "Installed configurations"
}

# Function to configure firewall
configure_firewall() {
    print_status "Configuring firewall..."
    
    if command -v ufw &> /dev/null; then
        # Ubuntu/Debian UFW
        ufw allow 69/udp
        print_success "Configured UFW firewall"
    elif command -v firewall-cmd &> /dev/null; then
        # CentOS/RHEL firewalld
        firewall-cmd --permanent --add-service=tftp
        firewall-cmd --reload
        print_success "Configured firewalld"
    elif command -v iptables &> /dev/null; then
        # Generic iptables
        iptables -A INPUT -p udp --dport 69 -j ACCEPT
        print_warning "Configured iptables (rules may not persist)"
    else
        print_warning "No firewall detected, please configure manually"
    fi
}

# Function to create sample files
create_sample_files() {
    print_status "Creating sample files..."
    
    # Create sample TFTP files
    echo "This is a sample TFTP file" > $TFTP_HOME/sample.txt
    echo "TFTP server is working correctly" > $TFTP_HOME/README.txt
    
    # Set ownership
    chown $TFTP_USER:$TFTP_GROUP $TFTP_HOME/*
    
    print_success "Created sample files"
}

# Function to start services
start_services() {
    print_status "Starting services..."
    
    if command -v systemctl &> /dev/null; then
        # Start main service
        systemctl start simple-tftpd
        systemctl enable simple-tftpd
        
        print_success "Started systemd service"
    else
        # Start init.d service
        /etc/init.d/simple-tftpd start
        
        print_success "Started init.d service"
    fi
}

# Function to verify installation
verify_installation() {
    print_status "Verifying installation..."
    
    # Check if binary exists
    if [[ ! -f $TFTP_BIN ]]; then
        print_error "Binary not found at $TFTP_BIN"
        return 1
    fi
    
    # Check if service is running
    if command -v systemctl &> /dev/null; then
        if systemctl is-active --quiet simple-tftpd; then
            print_success "Service is running"
        else
            print_error "Service is not running"
            return 1
        fi
    else
        if pgrep -f simple-tftpd > /dev/null; then
            print_success "Service is running"
        else
            print_error "Service is not running"
            return 1
        fi
    fi
    
    # Test TFTP connection
    if command -v tftp &> /dev/null; then
        timeout 5 tftp localhost 69 <<< "get sample.txt" > /dev/null 2>&1
        if [[ $? -eq 0 ]]; then
            print_success "TFTP connection test passed"
        else
            print_warning "TFTP connection test failed (this may be normal)"
        fi
    fi
    
    print_success "Installation verification completed"
}

# Function to display post-install information
display_post_install_info() {
    echo
    echo "=========================================="
    echo "Simple TFTP Daemon Installation Complete!"
    echo "=========================================="
    echo
    echo "Service Information:"
    echo "  - Binary: $TFTP_BIN"
    echo "  - Config: $TFTP_CONFIG_DIR"
    echo "  - Root: $TFTP_HOME"
    echo "  - Logs: $TFTP_LOG_DIR"
    echo "  - User: $TFTP_USER"
    echo
    echo "Service Management:"
    if command -v systemctl &> /dev/null; then
        echo "  - Start: systemctl start simple-tftpd"
        echo "  - Stop: systemctl stop simple-tftpd"
        echo "  - Status: systemctl status simple-tftpd"
        echo "  - Logs: journalctl -u simple-tftpd -f"
    else
        echo "  - Start: /etc/init.d/simple-tftpd start"
        echo "  - Stop: /etc/init.d/simple-tftpd stop"
        echo "  - Status: /etc/init.d/simple-tftpd status"
    fi
    echo
    echo "Configuration:"
    echo "  - Edit: $TFTP_CONFIG_DIR/simple-tftpd.conf"
    echo "  - Test: simple-tftpd test --config $TFTP_CONFIG_DIR/simple-tftpd.conf"
    echo
    echo "Next Steps:"
    echo "  1. Review configuration in $TFTP_CONFIG_DIR"
    echo "  2. Add your TFTP files to $TFTP_HOME"
    echo "  3. Test the service with: tftp localhost 69"
    echo "  4. Check logs in $TFTP_LOG_DIR"
    echo
}

# Main installation function
main() {
    print_status "Starting Simple TFTP Daemon installation..."
    
    # Check prerequisites
    check_root
    detect_os
    
    # Install dependencies
    install_dependencies
    
    # Create user and directories
    create_tftp_user
    create_directories
    
    # Install services
    install_systemd_services
    install_initd_scripts
    
    # Install configurations
    install_configurations
    
    # Configure firewall
    configure_firewall
    
    # Create sample files
    create_sample_files
    
    # Start services
    start_services
    
    # Verify installation
    verify_installation
    
    # Display information
    display_post_install_info
    
    print_success "Installation completed successfully!"
}

# Run main function
main "$@"
