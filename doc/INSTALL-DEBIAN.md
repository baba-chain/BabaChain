# BabaChain Core Debian/Ubuntu Installation Guide

This guide covers installation of BabaChain Core on Debian and Ubuntu systems.

## Package Installation

### Using Pre-built Packages

If you have access to pre-built BabaChain packages:

```bash
# Add BabaChain repository (if available)
# sudo add-apt-repository ppa:babachain/babachain
# sudo apt update

# Install BabaChain packages
sudo apt install babachaind babachain-qt babachain-tx
```

### Manual Installation from Source

#### 1. Install Dependencies

```bash
# Build dependencies
sudo apt-get update
sudo apt-get install build-essential libtool autotools-dev automake pkg-config bsdmainutils bison python3

# Core dependencies
sudo apt-get install libevent-dev libboost-dev libsqlite3-dev

# Optional: Berkeley DB for legacy wallet support
sudo apt-get install libdb-dev libdb++-dev

# Optional: GUI dependencies
sudo apt-get install qtbase5-dev qttools5-dev qttools5-dev-tools qtwayland5

# Optional: Additional features
sudo apt-get install libminiupnpc-dev libnatpmp-dev libzmq3-dev libqrencode-dev libgmp-dev
```

#### 2. Build BabaChain Core

```bash
# Clone the repository
git clone https://github.com/baba-chain/babachain.git
cd babachain

# Build
./autogen.sh
./configure
make -j$(nproc)

# Optional: Run tests
make check

# Install system-wide
sudo make install
```

#### 3. Install Desktop Integration

```bash
# Install desktop file
sudo desktop-file-install contrib/debian/babachain-qt.desktop
sudo update-desktop-database

# Install icons
sudo cp share/pixmaps/babachain*.png /usr/share/pixmaps/
sudo cp share/pixmaps/babachain*.xpm /usr/share/pixmaps/

# Install protocol handler (KDE)
sudo cp contrib/debian/babachain-qt.protocol /usr/share/kde4/services/
```

## Configuration

### Initial Setup

1. Create configuration directory:
```bash
mkdir -p ~/.babachain
```

2. Create basic configuration file:
```bash
cat > ~/.babachain/babachain.conf << EOF
# BabaChain Core configuration
server=1
daemon=1
rpcuser=babachainrpc
rpcpassword=$(openssl rand -hex 32)
rpcallowip=127.0.0.1
EOF
```

### Running BabaChain

#### Command Line (daemon)
```bash
# Start daemon
babachaind -daemon

# Check status
babachain-cli getblockchaininfo

# Stop daemon
babachain-cli stop
```

#### GUI Application
```bash
# Start GUI
babachain-qt

# Or from applications menu: Applications > Office > BabaChain Core
```

## Systemd Service (Optional)

Create a systemd service for automatic startup:

```bash
sudo tee /etc/systemd/system/babachaind.service << EOF
[Unit]
Description=BabaChain Core daemon
After=network.target

[Service]
Type=forking
User=babachain
Group=babachain
WorkingDirectory=/home/babachain
ExecStart=/usr/local/bin/babachaind -daemon -conf=/home/babachain/.babachain/babachain.conf -datadir=/home/babachain/.babachain
ExecStop=/usr/local/bin/babachain-cli -conf=/home/babachain/.babachain/babachain.conf stop
Restart=always
RestartSec=30

[Install]
WantedBy=multi-user.target
EOF

# Create babachain user
sudo useradd -r -m -s /bin/bash babachain

# Enable and start service
sudo systemctl enable babachaind
sudo systemctl start babachaind
```

## Troubleshooting

### Common Issues

1. **Build fails with missing dependencies**
   - Ensure all build dependencies are installed
   - Check `./configure` output for missing libraries

2. **GUI doesn't start**
   - Verify Qt5 dependencies are installed
   - Check X11/Wayland display settings

3. **Network connectivity issues**
   - Check firewall settings (port 9999 for mainnet)
   - Verify network configuration in babachain.conf

### Log Files

- Daemon logs: `~/.babachain/debug.log`
- System service logs: `sudo journalctl -u babachaind`

### Getting Help

- Documentation: https://docs.babachain.org/
- Community: https://discord.gg/babachain
- Issues: https://github.com/baba-chain/babachain/issues