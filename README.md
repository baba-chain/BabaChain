# BabaChain Core

[![Build Status](https://github.com/baba-chain/babachain/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/baba-chain/babachain/tree/master)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

> A next-generation Proof-of-Stake blockchain with fair economics and sustainable rewards

## What is BabaChain?

BabaChain is a modern cryptocurrency built from the ground up with Proof-of-Stake consensus, designed to be energy-efficient, fair, and sustainable. Unlike traditional cryptocurrencies with massive premines or unfair distributions, BabaChain allocates 90% of its supply to community staking rewards.

### 🚀 Key Features

- **⚡ Proof-of-Stake Consensus** - Energy efficient, fast, and secure
- **🎯 Fair Distribution** - Only 10% premine, 90% for community rewards  
- **📈 Progressive Economics** - Predictable reward reduction system
- **🔒 Secure Staking** - Minimum 1,000 BabaChain stake requirement
- **⏱️ Fast Blocks** - 2.5 minute average block time
- **🌱 Sustainable** - Designed for long-term network health

## 📊 Economic Model

| Parameter | Value |
|-----------|-------|
| **Total Supply** | 210,000,000 BabaChain |
| **Premine** | 20,000,000 (10%) |
| **Staking Pool** | 190,000,000 (90%) |
| **Initial Block Reward** | 200 BabaChain |
| **Reduction Rate** | 25% every 20M coins |
| **Block Time** | 2.5 minutes |

### 💰 Reward Schedule

The block rewards follow a predictable reduction schedule:

```
Block Reward Progression:
200 → 150 → 112.5 → 84.375 → 63.28 → 47.46 → 35.60 → 26.70 → 20.02 → 15.02
```

**Reduction Points:**
- At 40M total supply: 200 → 150 BabaChain
- At 60M total supply: 150 → 112.5 BabaChain  
- At 80M total supply: 112.5 → 84.375 BabaChain
- And so on...

## 🏗️ Getting Started

### Prerequisites

- C++17 compatible compiler
- CMake 3.16+
- Boost libraries
- OpenSSL
- libevent

### Building from Source

#### Linux/macOS
```bash
git clone https://github.com/baba-chain/babachain.git
cd babachain
./autogen.sh
./configure
make -j$(nproc)
```

#### Windows
See [build-windows.md](doc/build-windows.md) for detailed instructions.

### Running BabaChain

```bash
# Start the daemon
./src/babachaind

# Start with GUI
./src/qt/babachain-qt
```

## 🥩 Staking Guide

### Requirements
- Minimum 1,000 BabaChain
- Coins must be mature (8+ hours old)
- Wallet must be unlocked for staking

### How to Stake
1. Ensure you have at least 1,000 BabaChain
2. Keep your wallet online and unlocked
3. Enable staking in the wallet settings
4. Wait for your coins to mature (8 hours)
5. Start earning rewards!

### Staking Rewards
Your staking rewards depend on:
- Your stake amount (more stake = higher probability)
- Network participation (total staked coins)
- Current block reward (reduces over time)

## 🔧 Configuration

### babachain.conf Example
```ini
# Network
listen=1
server=1
daemon=1

# Staking
staking=1
stakeminconfirmations=1

# RPC
rpcuser=yourusername
rpcpassword=yourpassword
rpcallowip=127.0.0.1
```

## 🧪 Testing

### Unit Tests
```bash
make check
```

### Functional Tests
```bash
test/functional/test_runner.py
```

## 📚 Documentation

- [Build Instructions](doc/)
- [Configuration Guide](doc/configuration.md)
- [API Reference](doc/api.md)
- [Staking Guide](doc/staking.md)
- [Whitepaper](whitepaper.md)

## 🤝 Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Development Process
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## 🛡️ Security

BabaChain takes security seriously. If you discover a security vulnerability, please email security@babachain.org instead of creating a public issue.

## 📄 License

BabaChain Core is released under the [MIT License](LICENSE).

## 🌐 Community

- **Website**: https://www.babachain.org
- **Discord**: https://discord.gg/babachain
- **Twitter**: https://twitter.com/babachainorg
- **Telegram**: https://t.me/babachain
- **Reddit**: https://reddit.com/r/babachain

## ⚠️ Disclaimer

BabaChain is experimental software. Use at your own risk. Always do your own research before investing in any cryptocurrency.

---

**Built with ❤️ by the BabaChain Community**