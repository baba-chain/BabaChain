#!/bin/bash
# BabaChain Mainnet Node Deployment Script

set -e

echo "🚀 Deploying BabaChain Mainnet Node..."

# Configuration
BABACHAIN_USER="babachain"
BABACHAIN_HOME="/home/$BABACHAIN_USER"
BABACHAIN_DATA="$BABACHAIN_HOME/.babachain"
BABACHAIN_CONF="$BABACHAIN_DATA/babachain.conf"

# Create user if not exists
if ! id "$BABACHAIN_USER" &>/dev/null; then
    echo "Creating BabaChain user..."
    useradd -m -s /bin/bash $BABACHAIN_USER
fi

# Create data directory
sudo -u $BABACHAIN_USER mkdir -p $BABACHAIN_DATA

# Copy configuration
sudo -u $BABACHAIN_USER cp config/babachain-mainnet.conf $BABACHAIN_CONF

# Set proper permissions
chmod 600 $BABACHAIN_CONF
chown $BABACHAIN_USER:$BABACHAIN_USER $BABACHAIN_CONF

# Install systemd service
cat > /etc/systemd/system/babachaind.service << 'EOF'
[Unit]
Description=BabaChain daemon
After=network.target

[Service]
Type=forking
User=babachain
Group=babachain
WorkingDirectory=/home/babachain
ExecStart=/usr/local/bin/babachaind -daemon -conf=/home/babachain/.babachain/babachain.conf
ExecStop=/usr/local/bin/babachain-cli stop
Restart=always
RestartSec=30
TimeoutStopSec=60
KillMode=process
PrivateTmp=true

[Install]
WantedBy=multi-user.target
EOF

# Enable and start service
systemctl daemon-reload
systemctl enable babachaind
systemctl start babachaind

echo "✅ BabaChain mainnet node deployed successfully!"
echo "📊 Check status with: systemctl status babachaind"
echo "📋 View logs with: journalctl -u babachaind -f"
