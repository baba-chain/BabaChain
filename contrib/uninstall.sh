#!/bin/bash
# BabaChain Core Uninstallation Script
# This script removes BabaChain Core binaries and desktop integration

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
if [[ $EUID -eq 0 ]]; then
    print_error "This script should not be run as root"
    exit 1
fi

print_warning "This will remove BabaChain Core from your system"
print_warning "Your wallet and configuration files will NOT be deleted"
echo ""
read -p "Are you sure you want to continue? (y/N): " -n 1 -r
echo ""

if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    print_status "Uninstallation cancelled"
    exit 0
fi

print_status "Removing BabaChain Core..."

# Stop any running processes
if pgrep -x "babachaind" > /dev/null; then
    print_status "Stopping babachaind..."
    babachain-cli stop 2>/dev/null || true
    sleep 3
fi

if pgrep -x "babachain-qt" > /dev/null; then
    print_status "Stopping babachain-qt..."
    pkill -x "babachain-qt" 2>/dev/null || true
fi

# Remove binaries
BINARIES=("babachaind" "babachain-cli" "babachain-qt" "babachain-tx")
for binary in "${BINARIES[@]}"; do
    if [[ -f "/usr/local/bin/$binary" ]]; then
        sudo rm -f "/usr/local/bin/$binary"
        print_status "Removed $binary"
    fi
done

# Remove desktop integration
if [[ -f "/usr/share/applications/babachain-qt.desktop" ]]; then
    sudo rm -f "/usr/share/applications/babachain-qt.desktop"
    print_status "Removed desktop file"
fi

# Remove icons
sudo rm -f /usr/share/pixmaps/babachain*.png 2>/dev/null || true
sudo rm -f /usr/share/pixmaps/babachain*.xpm 2>/dev/null || true
print_status "Removed icons"

# Remove KDE protocol handler
if [[ -f "/usr/share/kde4/services/babachain-qt.protocol" ]]; then
    sudo rm -f "/usr/share/kde4/services/babachain-qt.protocol"
    print_status "Removed KDE protocol handler"
fi

# Update desktop database
if command -v update-desktop-database >/dev/null 2>&1; then
    sudo update-desktop-database 2>/dev/null || true
    print_status "Updated desktop database"
fi

# Remove systemd service if it exists
if [[ -f "/etc/systemd/system/babachaind.service" ]]; then
    sudo systemctl stop babachaind 2>/dev/null || true
    sudo systemctl disable babachaind 2>/dev/null || true
    sudo rm -f "/etc/systemd/system/babachaind.service"
    sudo systemctl daemon-reload
    print_status "Removed systemd service"
fi

print_status "BabaChain Core has been successfully removed"
print_status ""
print_warning "Your configuration and wallet files are still located at:"
print_warning "  ~/.babachain/"
print_warning ""
print_warning "To completely remove all BabaChain data, run:"
print_warning "  rm -rf ~/.babachain/"
print_warning ""
print_warning "WARNING: This will delete your wallet and all blockchain data!"