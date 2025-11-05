#!/bin/bash
# BabaChain Genesis Block Generation Script

set -e

echo "🎯 Generating BabaChain Genesis Block..."

# Build babachaind if not exists
if [ ! -f "src/babachaind" ]; then
    echo "Building BabaChain daemon..."
    make -j$(nproc) babachaind
fi

# Generate genesis block
echo "Generating genesis block with 20M premine..."
./src/babachaind -printtoconsole -regtest -gen=0 -connect=0 -listen=0 -rpcuser=genesis -rpcpassword=genesis &
DAEMON_PID=$!

# Wait for daemon to start
sleep 5

# Generate genesis block
GENESIS_HASH=$(./src/babachain-cli -regtest -rpcuser=genesis -rpcpassword=genesis getblockhash 0)
GENESIS_BLOCK=$(./src/babachain-cli -regtest -rpcuser=genesis -rpcpassword=genesis getblock $GENESIS_HASH)

echo "Genesis Block Hash: $GENESIS_HASH"
echo "Genesis Block Data: $GENESIS_BLOCK"

# Save genesis data
echo "$GENESIS_BLOCK" > config/mainnet_genesis_block.json

# Stop daemon
kill $DAEMON_PID

echo "✅ Genesis block generated successfully!"
echo "📄 Genesis data saved to: config/mainnet_genesis_block.json"
