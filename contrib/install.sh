#!/bin/bash
# BabaChain Core Installation Script
# This script installs BabaChain Core binaries and desktop integration

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

# Detect OS
if [[ -f /etc/os-release ]]; then
    . /etc/os-release
    OS=$NAME
    VER=$VERSION_ID
else
    print_error "Cannot detect OS version"
    exit 1
fi

print_status "Detected OS: $OS $VER"

# Check if binaries exist
BINARIES=("babachaind" "babachain-cli" "babachain-qt" "babachain-tx")
MISSING_BINARIES=()

for binary in "${BINARIES[@]}"; do
    if [[ ! -f "src/$binary" ]] && [[ ! -f "src/qt/$binary" ]]; then
        MISSING_BINARIES+=("$binary")
    fi
done

if [[ ${#MISSING_BINARIES[@]} -gt 0 ]]; then
    print_error "Missing binaries: ${MISSING_BINARIES[*]}"
    print_error "Please build BabaChain Core first using 'make'"
    exit 1
fi

print_status "All required binaries found"

# Install binaries
print_status "Installing BabaChain Core binaries..."

sudo mkdir -p /usr/local/bin

# Install daemon and CLI
if [[ -f "src/babachaind" ]]; then
    sudo cp src/babachaind /usr/local/bin/
    print_status "Installed babachaind"
fi

if [[ -f "src/babachain-cli" ]]; then
    sudo cp src/babachain-cli /usr/local/bin/
    print_status "Installed babachain-cli"
fi

if [[ -f "src/babachain-tx" ]]; then
    sudo cp src/babachain-tx /usr/local/bin/
    print_status "Installed babachain-tx"
fi

# Install GUI if available
if [[ -f "src/qt/babachain-qt" ]]; then
    sudo cp src/qt/babachain-qt /usr/local/bin/
    print_status "Installed babachain-qt"
    
    # Install desktop integration
    print_status "Installing desktop integration..."
    
    # Install desktop file
    if [[ -f "contrib/debian/babachain-qt.desktop" ]]; then
        sudo desktop-file-install contrib/debian/babachain-qt.desktop
        print_status "Installed desktop file"
    fi
    
    # Install icons
    sudo mkdir -p /usr/share/pixmaps
    if [[ -d "share/pixmaps" ]]; then
        sudo cp share/pixmaps/babachain*.png /usr/share/pixmaps/ 2>/dev/null || true
        sudo cp share/pixmaps/babachain*.xpm /usr/share/pixmaps/ 2>/dev/null || true
        print_status "Installed icons"
    fi
    
    # Update desktop database
    if command -v update-desktop-database >/dev/null 2>&1; then
        sudo update-desktop-database
        print_status "Updated desktop database"
    fi
    
    # Install KDE protocol handler
    if [[ -f "contrib/debian/babachain-qt.protocol" ]]; then
        sudo mkdir -p /usr/share/kde4/services
        sudo cp contrib/debian/babachain-qt.protocol /usr/share/kde4/services/
        print_status "Installed KDE protocol handler"
    fi
fi

# Set permissions
sudo chmod +x /usr/local/bin/babachain*

# Create configuration directory for current user
CONFIG_DIR="$HOME/.babachain"
if [[ ! -d "$CONFIG_DIR" ]]; then
    mkdir -p "$CONFIG_DIR"
    print_status "Created configuration directory: $CONFIG_DIR"
fi

# Create basic configuration file if it doesn't exist
CONFIG_FILE="$CONFIG_DIR/babachain.conf"
if [[ ! -f "$CONFIG_FILE" ]]; then
    cat > "$CONFIG_FILE" << EOF
# BabaChain Core configuration file
# Uncomment and modify as needed

# Network settings
#testnet=1
#regtest=1

# RPC settings
server=1
#rpcuser=babachainrpc
#rpcpassword=your_secure_password_here
#rpcallowip=127.0.0.1

# Connection settings
#maxconnections=125
#addnode=node1.babachain.org
#addnode=node2.babachain.org

# Logging
#debug=1
#printtoconsole=1
EOF
    print_status "Created sample configuration file: $CONFIG_FILE"
    print_warning "Please edit $CONFIG_FILE and set a secure RPC password"
fi

print_status "Installation completed successfully!"
print_status ""
print_status "You can now run:"
print_status "  babachaind          - Start BabaChain daemon"
print_status "  babachain-cli       - Command line interface"
print_status "  babachain-qt        - GUI wallet"
print_status "  babachain-tx        - Transaction utility"
print_status ""
print_status "Configuration file: $CONFIG_FILE"
print_status "Data directory: $CONFIG_DIR"
print_status ""
print_warning "Remember to:"
print_warning "1. Set a secure RPC password in the configuration file"
print_warning "2. Backup your wallet.dat file regularly"
print_warning "3. Keep your private keys secure"