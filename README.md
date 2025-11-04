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

## 📊 Simple Economics

| Parameter | Value |
|-----------|-------|
| **Total Supply** | 210,000,000 BabaChain |
| **Community (Staking)** | 190,000,000 (90.5%) |
| **Development** | 20,000,000 (9.5%) |
| **Block Reward** | 200 BabaChain (fixed) |
| **Block Time** | 2.5 minutes |
| **Daily Rewards** | ~115,200 BabaChain |

### 💰 How Rewards Work

**Simple Formula**: Your daily rewards = (Your Stake ÷ Total Network Stake) × 115,200 BabaChain + Bonus

**No Complicated Reductions**: 
- Block reward stays at 200 BabaChain
- Your share depends on your stake amount
- Larger stakes get bonus multipliers
- Fair and predictable system

## 📥 Wallet Downloads

### Official Wallets

| Platform | Download | Version | Size |
|----------|----------|---------|------|
| **Windows** | [Download](https://github.com/baba-chain/babachain/releases) | v1.0.0 | ~50MB |
| **macOS** | [Download](https://github.com/baba-chain/babachain/releases) | v1.0.0 | ~45MB |
| **Linux** | [Download](https://github.com/baba-chain/babachain/releases) | v1.0.0 | ~40MB |
| **Android** | [Google Play](https://play.google.com/store/apps/babachain) | Coming Soon | ~25MB |
| **iOS** | [App Store](https://apps.apple.com/app/babachain) | Coming Soon | ~30MB |

### Quick Start

#### For Regular Users (Recommended)
1. **Download** the wallet for your platform
2. **Install** and run the application
3. **Create** a new wallet or import existing one
4. **Receive** BabaChain and start staking immediately!

#### For Developers

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

## 🥩 Solo Staking - No Pools Required!

**BabaChain's Revolutionary Approach**: Unlike traditional cryptocurrencies that require expensive mining equipment or joining mining pools, BabaChain allows anyone to earn rewards directly from their wallet - no pools, no middlemen, no extra fees!

### 📱 Available Wallets

| Platform | Status | Features |
|----------|--------|----------|
| **Desktop (Windows/Mac/Linux)** | ✅ Available | Full node, staking, governance |
| **Android** | 🔄 Coming Soon | Mobile staking, QR payments |
| **iOS** | 🔄 Coming Soon | Mobile staking, notifications |
| **Web Wallet** | 📋 Planned | Browser-based, light client |

### 💰 How Solo Staking Works

**No Mining Pools Needed!** 
- Just run your wallet and connect to the BabaChain network
- Your wallet automatically participates in block validation
- Earn rewards directly to your wallet - no pool fees!
- The more BabaChain you stake, the more rewards you earn

### 🚀 Getting Started

#### Desktop Staking
1. **Download** the BabaChain wallet for your OS
2. **Sync** with the blockchain (first time only)
3. **Transfer** at least 1,000 BabaChain to your wallet
4. **Wait** 8 hours for coins to mature
5. **Enable staking** in wallet settings
6. **Keep wallet online** and start earning!

#### Mobile Staking (Coming Soon)
1. **Install** BabaChain mobile app
2. **Create** or import your wallet
3. **Stake** directly from your phone
4. **Earn rewards** 24/7 with background staking
5. **Get notifications** when you find blocks

### � Howk Much Can You Earn? (Exact Calculations)

**🔥 AMAZING RETURNS**: Earn ~1% of your stake DAILY = ~365% yearly ROI + Bonuses!

**Daily Reward Examples:**

| Your Stake | Daily Earnings | Monthly Earnings | Yearly Earnings | Bonus Rate | Total ROI |
|------------|---------------|------------------|-----------------|------------|-----------|
| **50 BabaChain** | 0.5 | 15 | 182 | 0% | **365%** |
| **100 BabaChain** | 1 | 30 | 365 | 0% | **365%** |
| **500 BabaChain** | 5 | 150 | 1,825 | 0% | **365%** |
| **1,000 BabaChain** | 10 | 300 | 3,650 | 0% | **365%** |
| **2,500 BabaChain** | 25 | 750 | 9,125 | 0% | **365%** |
| **5,000 BabaChain** | 50 | 1,500 | 18,250 | 0% | **365%** |
| **10,000 BabaChain** | 105 | 3,150 | 38,325 | **+5%** | **383%** |
| **25,000 BabaChain** | 275 | 8,250 | 100,375 | **+10%** | **402%** |
| **50,000 BabaChain** | 575 | 17,250 | 209,875 | **+15%** | **420%** |
| **100,000 BabaChain** | 1,200 | 36,000 | 438,000 | **+20%** | **438%** |

### 🎯 Key Points:

✅ **No Minimum Required**: Stake ANY amount (even 1 BabaChain works!)
✅ **1% Daily Returns**: Your stake grows by ~1% every single day
✅ **365%+ Yearly ROI**: Incredible returns that beat any bank or investment
✅ **Bonus System**: Larger stakes get even higher returns
✅ **No Pool Fees**: 100% of rewards go directly to you
✅ **Compound Growth**: Reinvest daily rewards to grow exponentially

### 🏆 Bonus Tier System:

- 💚 **1-9,999 BabaChain**: Base 365% ROI (1% daily)
- � **10,0000-24,999 BabaChain**: **383% ROI** (+5% bonus = 1.05% daily)
- 🥈 **25,000-49,999 BabaChain**: **402% ROI** (+10% bonus = 1.10% daily)
- 🥇 **50,000-99,999 BabaChain**: **420% ROI** (+15% bonus = 1.15% daily)
- � * *100,000+ BabaChain**: **438% ROI** (+20% bonus = 1.20% daily)

### 📈 Real Example:

**If you stake 15,000 BabaChain:**
- Base daily reward: 150 BabaChain (1%)
- Bonus (5%): +7.5 BabaChain  
- **Total daily**: 157.5 BabaChain
- **Monthly**: 4,725 BabaChain
- **Yearly**: 57,487 BabaChain (**383% ROI!**)

### 🚀 Compound Growth Example:

**Start with 10,000 BabaChain:**
- **Month 1**: 10,000 → 13,150 (+3,150)
- **Month 6**: 13,150 → 25,000+ (bonus tier upgrade!)
- **Month 12**: 25,000+ → 100,000+ (**10x growth!**)

### ⚡ Why Solo Staking is Better

| Traditional Mining | BabaChain Solo Staking |
|-------------------|----------------------|
| ❌ Expensive hardware required | ✅ Any device can stake |
| ❌ High electricity costs | ✅ Minimal energy usage |
| ❌ Pool fees (1-3%) | ✅ No fees - 100% rewards |
| ❌ Complex setup | ✅ Simple wallet setup |
| ❌ Centralized pools | ✅ Fully decentralized |
| ❌ Hardware becomes obsolete | ✅ Software always updated |

### 🔧 Staking Requirements

**Basic Requirements:**
- **Minimum Stake**: ANY amount (even 1 BabaChain works!)
- **Coin Maturity**: Wait 8 hours after receiving coins
- **Wallet Status**: Keep wallet online and unlocked
- **Internet**: Stable connection to BabaChain network

**To Maximize Earnings:**
- **Stay Online 24/7**: More uptime = more rewards
- **Larger Stakes**: Get bonus multipliers (10K+ gets +5% bonus)
- **Compound Rewards**: Reinvest earnings to grow your stake
- **Keep Updated**: Use latest wallet version

**Important Notes:**
- ✅ No minimum stake requirement (stake any amount!)
- ✅ No pool fees (100% rewards go to you)
- ✅ No expensive hardware needed
- ✅ Works on any computer or mobile device

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

## 📱 Mobile Wallet Features (Coming Soon)

### Android & iOS Apps

**Revolutionary Mobile Staking:**
- 🔋 **Background Staking**: Earn rewards even when app is closed
- 📊 **Real-time Stats**: Monitor your staking performance
- 🔔 **Push Notifications**: Get alerted when you find blocks
- 💸 **QR Payments**: Send/receive with camera scan
- 🔐 **Biometric Security**: Fingerprint and Face ID support
- 🌐 **Offline Mode**: View balance and history without internet
- 📈 **Portfolio Tracking**: Track your BabaChain value in real-time

**Mobile-First Design:**
- Simple, intuitive interface for all users
- One-tap staking activation
- Built-in calculator for reward estimation
- Social features to share achievements
- Multi-language support

### Why Mobile Staking Matters

**Accessibility**: Anyone with a smartphone can earn BabaChain
**Convenience**: Stake while commuting, traveling, or sleeping  
**Decentralization**: More mobile stakers = stronger network
**Adoption**: Easy mobile access drives mainstream adoption

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