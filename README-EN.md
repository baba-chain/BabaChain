# BabaChain Core

![BabaChain Logo](logo.png)

[![Build Status](https://github.com/baba-chain/babachain/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/baba-chain/babachain/tree/master)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

> A next-generation Proof-of-Stake blockchain with fair economics and sustainable rewards

## What is BabaChain?

BabaChain is a modern cryptocurrency built from the ground up with Proof-of-Stake consensus, designed to be energy-efficient, fair, and sustainable. Unlike traditional cryptocurrencies with massive premines or unfair distributions, BabaChain allocates 90% of its supply to community staking rewards.

### 🚀 Key Features

- **⚡ Proof-of-Stake Consensus** - Energy efficient, fast, and secure
- **🎯 Fair Distribution** - Only 10% premine, 90% for community rewards  
- **📈 Gradual Bonus System** - Smooth reward progression based on stake size
- **🔒 Secure Staking** - Any amount can stake (even 1 BabaChain!)
- **⏱️ Fast Blocks** - 2.5 minute average block time
- **🌱 Sustainable** - Designed for long-term network health

## 📊 Economics & Supply

| Parameter | Value |
|-----------|-------|
| **Initial Supply** | 210,000,000 BabaChain |
| **Maximum Supply** | 1,000,000,000 BabaChain (hard cap) |
| **Premine** | 20,000,000 (~10%) |
| **Initial Staking Pool** | 190,000,000 (~90%) |
| **Extended Staking Pool** | 790,000,000 (210M to 1B) |
| **Block Time** | 2.5 minutes |
| **Staking ROI** | ~365% annually (1% daily) + gradual bonuses |

### 💰 How Rewards Work

**Simple Formula**: Your daily rewards = Your Stake × 1% + Gradual Bonus

**No Complex Math**: Just earn ~1% of your stake daily, plus bonuses for larger stakes!

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

**Revolutionary Auto-Node System!** 
- Every wallet automatically becomes a network node
- No manual setup - just install and run!
- Your wallet auto-connects to the BabaChain network
- Automatically participates in block validation and earns rewards
- The more users join, the stronger and more decentralized the network becomes
- Earn rewards directly to your wallet - no pool fees, no middlemen!

### 📱 Available Wallets

| Platform | Status | Features |
|----------|--------|----------|
| **Desktop (Windows/Mac/Linux)** | ✅ Available | Full node, staking, governance |
| **Android** | 🔄 Coming Soon | Mobile staking, QR payments |
| **iOS** | 🔄 Coming Soon | Mobile staking, notifications |
| **Web Wallet** | 📋 Planned | Browser-based, light client |

### � Get ting Started

#### Desktop Staking
1. **Download** the BabaChain wallet for your OS
2. **Sync** with the blockchain (first time only)
3. **Transfer** BabaChain to your wallet (any amount!)
4. **Wait** 8 hours for coins to mature
5. **Enable staking** in wallet settings
6. **Keep wallet online** and start earning!

#### Mobile Staking (Coming Soon)
1. **Install** BabaChain mobile app
2. **Create** or import your wallet
3. **Stake** directly from your phone
4. **Earn rewards** 24/7 with background staking
5. **Get notifications** when you find blocks

### 💰 How Much Can You Earn? (Gradual Bonus System!)

**🔥 AMAZING RETURNS**: Earn ~1% of your stake DAILY = ~365% yearly ROI + Gradual Bonuses!

**Daily Reward Examples:**

| Your Stake | Base Daily (1%) | Gradual Bonus | Total Daily | Monthly | Yearly | ROI |
|------------|----------------|---------------|-------------|---------|--------|-----|
| **100 BabaChain** | 1.0 | +0.05 | 1.05 | 31.5 | 383 | **383%** |
| **1,000 BabaChain** | 10.0 | +0.5 | 10.5 | 315 | 3,833 | **383%** |
| **5,000 BabaChain** | 50.0 | +12.5 | 62.5 | 1,875 | 22,813 | **456%** |
| **10,000 BabaChain** | 100.0 | +50 | 150 | 4,500 | 54,750 | **548%** |
| **25,000 BabaChain** | 250.0 | +208 | 458 | 13,750 | 167,175 | **669%** |
| **50,000 BabaChain** | 500.0 | +625 | 1,125 | 33,750 | 410,625 | **821%** |
| **100,000 BabaChain** | 1,000.0 | +2,000 | 3,000 | 90,000 | 1,095,000 | **1,095%** |

### 🎯 Key Points:

✅ **No Minimum Required**: Stake ANY amount (even 1 BabaChain works!)
✅ **1% Daily Returns**: Your stake grows by ~1% every single day
✅ **365%+ Yearly ROI**: Incredible returns that beat any bank or investment
✅ **Gradual Bonus System**: Larger stakes get progressively higher bonuses
✅ **No Pool Fees**: 100% of rewards go directly to you
✅ **Compound Growth**: Reinvest daily rewards to grow exponentially

### 🏆 Gradual Bonus System (Smooth Progression):

**No Fixed Tiers - Continuous Bonus Growth!**

- 💚 **1-10,000 BabaChain**: 0% to 5% bonus (gradual increase)
- 🚀 **10,000-100,000 BabaChain**: 5% to 20% bonus (gradual increase)  
- 💎 **100,000+ BabaChain**: Maximum 20% bonus

**How It Works:**
- Every additional BabaChain increases your bonus slightly
- No sudden jumps or unfair tier cutoffs
- Smooth mathematical progression rewards growth
- The more you stake, the higher your daily percentage

**Bonus Examples:**
- **1,000 BabaChain**: ~0.5% bonus = 1.005% daily (367% ROI)
- **5,000 BabaChain**: ~2.5% bonus = 1.025% daily (374% ROI)
- **10,000 BabaChain**: 5% bonus = 1.05% daily (383% ROI)
- **25,000 BabaChain**: ~8.3% bonus = 1.083% daily (395% ROI)
- **50,000 BabaChain**: ~12.5% bonus = 1.125% daily (411% ROI)
- **75,000 BabaChain**: ~16.7% bonus = 1.167% daily (426% ROI)
- **100,000 BabaChain**: 20% bonus = 1.20% daily (438% ROI)

### 📈 Real Example:

**If you stake 15,000 BabaChain:**
- Base daily reward: 150 BabaChain (1%)
- Gradual bonus (~5.6%): +8.4 BabaChain  
- **Total daily**: 158.4 BabaChain
- **Monthly**: 4,752 BabaChain
- **Yearly**: 57,816 BabaChain (**385% ROI!**)

### 🚀 Compound Growth Example:

**Start with 10,000 BabaChain:**
- **Month 1**: 10,000 → 14,500 (+4,500)
- **Month 6**: 14,500 → 35,000+ (bonus increases!)
- **Month 12**: 35,000+ → 150,000+ (**15x growth!**)


### 🌐 Network Growth = Everyone Wins

**The More Users, The Stronger The Network:**
- Every new wallet = New network node
- More nodes = Better security and speed
- Larger network = Higher BabaChain value
- Your rewards grow as network grows!

**Viral Growth Mechanics:**
- � ***Easy Mobile Apps**: Anyone can start staking in 30 seconds
- 🔄 **Auto-Everything**: No technical setup required
- 💰 **365%+ ROI**: Incredible returns attract more users
- 🎯 **Referral Bonuses**: Earn extra for bringing friends
- 🏆 **Social Features**: Share achievements, compete with friends

### ⚡ Why BabaChain's Auto-Node System is Revolutionary

| Traditional Crypto | BabaChain Auto-Node System |
|-------------------|---------------------------|
| ❌ Expensive mining hardware | ✅ Any device becomes a node |
| ❌ High electricity costs | ✅ Minimal energy usage |
| ❌ Pool fees (1-3%) | ✅ No fees - 100% rewards |
| ❌ Complex node setup | ✅ Zero setup - auto-connects |
| ❌ Centralized mining pools | ✅ Every wallet = decentralized node |
| ❌ Hardware becomes obsolete | ✅ Software auto-updates |
| ❌ Need technical knowledge | ✅ Anyone can participate |
| ❌ Network controlled by few | ✅ Network grows with every user |

### 🔧 Staking Requirements

**Basic Requirements:**
- **Minimum Stake**: ANY amount (even 1 BabaChain works!)
- **Coin Maturity**: Wait 8 hours after receiving coins
- **Wallet Status**: Keep wallet online and unlocked
- **Internet**: Stable connection to BabaChain network

**To Maximize Earnings:**
- **Stay Online 24/7**: More uptime = more rewards
- **Larger Stakes**: Get gradual bonus increases
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

## 📱 Mobile Wallet Features (Coming Soon)

### Android & iOS Apps

**Revolutionary Mobile Staking:**
- 🔋 **Background Staking**: Earn rewards even when app is closed
- 📊 **Real-time Stats**: Monitor your staking performance
- 🔔 **Push Notifications**: Get alerted when you find blocks
- 💸 **QR Payments**: Send/receive with camera scan
- � **Biomretric Security**: Fingerprint and Face ID support
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