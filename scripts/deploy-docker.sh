#!/bin/bash

# Docker deployment script for Simple Secure FTP Daemon

set -e

# Default values
PROFILE="runtime"
CONFIG_DIR=""
LOG_DIR=""
DATA_DIR=""
CLEAN=false
FORCE=false

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  -p, --profile PROFILE    Deployment profile (dev, runtime)"
    echo "  -c, --config DIR         Configuration directory"
    echo "  -l, --logs DIR           Log directory"
    echo "  -d, --data DIR           FTP data directory"
    echo "  --clean                  Clean existing containers before deployment"
    echo "  --force                  Force rebuild even if image exists"
    echo "  -h, --help               Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 -p runtime"
    echo "  $0 -p dev -c ./config -l ./logs -d ./data"
    echo "  $0 --clean --force"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -p|--profile)
            PROFILE="$2"
            shift 2
            ;;
        -c|--config)
            CONFIG_DIR="$2"
            shift 2
            ;;
        -l|--logs)
            LOG_DIR="$2"
            shift 2
            ;;
        -d|--data)
            DATA_DIR="$2"
            shift 2
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        --force)
            FORCE=true
            shift
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Check prerequisites
echo "Checking prerequisites..."
if ! command -v docker &> /dev/null; then
    echo "ERROR: Docker is not installed"
    exit 1
fi

if ! command -v docker-compose &> /dev/null; then
    echo "ERROR: Docker Compose is not installed"
    exit 1
fi

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo "Cleaning existing containers..."
    docker-compose down -v
    docker system prune -f
fi

# Create directories if they don't exist
if [ -n "$CONFIG_DIR" ] && [ ! -d "$CONFIG_DIR" ]; then
    echo "Creating configuration directory: $CONFIG_DIR"
    mkdir -p "$CONFIG_DIR"
fi

if [ -n "$LOG_DIR" ] && [ ! -d "$LOG_DIR" ]; then
    echo "Creating log directory: $LOG_DIR"
    mkdir -p "$LOG_DIR"
fi

if [ -n "$DATA_DIR" ] && [ ! -d "$DATA_DIR" ]; then
    echo "Creating FTP data directory: $DATA_DIR"
    mkdir -p "$DATA_DIR"
fi

# Build and deploy
echo "Deploying ssftpd with profile: $PROFILE"

if [ "$FORCE" = true ]; then
    echo "Force rebuilding images..."
    docker-compose build --no-cache
fi

# Deploy based on profile
case $PROFILE in
    "dev")
        echo "Starting development environment..."
        docker-compose --profile dev up -d dev
        echo "Development container started. Access with: docker-compose exec dev bash"
        ;;
    "runtime")
        echo "Starting runtime environment..."
        docker-compose --profile runtime up -d ssftpd
        echo "Runtime container started."
        ;;
    *)
        echo "ERROR: Unsupported profile: $PROFILE"
        exit 1
        ;;
esac

# Show status
echo ""
echo "Container status:"
docker-compose ps

echo ""
echo "To view logs: docker-compose logs -f"
echo "To stop: docker-compose down"
