# BabaChain Android Mobile Wallet

[![Build Status](https://github.com/baba-chain/BabaChain-AndroidMobileWallet/actions/workflows/android.yml/badge.svg)](https://github.com/baba-chain/babachain-android/actions) [![License](https://img.shields.io/badge/license-MIT-green)](https://github.com/baba-chain/babachain-android/blob/master/LICENSE) [![Release](https://img.shields.io/github/v/release/baba-chain/babachain-android)](https://github.com/baba-chain/babachain-android/releases) [![Platform](https://img.shields.io/badge/platform-Android-green)](https://github.com/baba-chain/babachain-android)

<p align="center" >
<img src="https://www.babachain.org/images/babachain_logo.png" alt="BabaChain Wallet logo" title="BabaChain Wallet" width="300">
</p>

*BabaChain Wallet* is a next-generation mobile wallet for the [BabaChain](https://babachain.org) Proof-of-Stake blockchain. There is no server to get hacked or go down, so you can always access your money and earn staking rewards.
Using [SPV](https://en.bitcoin.it/wiki/Thin_Client_Security#Header-Only_Clients) light node technology, *BabaChain Wallet* connects directly to the BabaChain network with the fast performance you need on a mobile device.
*BabaChain Wallet* is designed to protect you from malware, browser security holes, even physical theft. With fingerprint/face biometric authentication, hardware-backed keystore storage, app sandboxing,
and code signatures, *BabaChain Wallet* represents a significant security advantage over web and desktop wallets, and other mobile platforms.
Simplicity and earning potential are *BabaChain Wallet*'s core design principles. A simple backup phrase is all you need to restore your wallet on another device if yours is ever lost or broken.
Because *BabaChain Wallet* is deterministic, your balance, staking rewards, and transaction history can be recovered from just your backup phrase.

## 🚀 Revolutionary Features

### 💰 Mobile Staking with 365%+ APR
- **Background Staking**: Earn rewards 24/7 even when app is closed
- **Gradual Bonus System**: 1% daily base + up to 20% bonus for larger stakes
- **No Minimum**: Stake any amount (even 1 BABA works!)
- **Auto-Compounding**: Rewards automatically increase your stake
- **Real-time Notifications**: Get alerted when you earn rewards

### 🔒 Advanced Security
- **Fingerprint/Face ID**: Biometric authentication for all operations
- **Hardware Keystore**: Private keys stored in Android Keystore
- **Transaction Protection**: Biometric confirmation for all payments
- **Seed Phrase Security**: Encrypted backup with biometric access
- **Auto-lock**: Automatic wallet locking for security

### 📱 Android-Specific Features
- **Home Screen Widgets**: Real-time balance and staking info
- **Google Pay Integration**: Buy BabaChain instantly with one tap
- **Push Notifications**: Staking rewards, transactions, network status
- **Background Services**: Continuous staking and sync
- **NFC Support**: Tap-to-pay and receive functionality
- **Offline Mode**: View balance and history without internet

### ⚡ SPV Light Node
- **Fast Sync**: Lightweight blockchain synchronization
- **Direct Connection**: No intermediary servers or pools
- **Network Resilience**: Automatic peer discovery and reconnection
- **PoS Support**: Full Proof-of-Stake consensus integration
- **Real-time Updates**: Live network status and sync progress

## 📊 BabaChain Economics

*BabaChain Wallet* fully integrates BabaChain's fair and sustainable economics:

| Parameter | Value |
|-----------|-------|
| **Initial Supply** | 210,000,000 BABA |
| **Maximum Supply** | 1,000,000,000 BABA (hard cap) |
| **Premine** | 20,000,000 BABA (~10% - fair distribution) |
| **Community Rewards** | 190,000,000 BABA (~90% for stakers) |
| **Base Staking Rate** | 1% daily (365% APR) |
| **Gradual Bonuses** | Up to 20% additional for larger stakes |

### 🎯 Gradual Bonus System

The mobile wallet calculates your exact rewards using BabaChain's innovative gradual bonus system:

- **1-10,000 BABA**: 0% to 5% gradual bonus
- **10,000-100,000 BABA**: 5% to 20% gradual bonus  
- **100,000+ BABA**: Maximum 20% bonus

**Example**: 15,000 BABA stake earns 1.056% daily = **385% APR**

## Features

- **SPV Light Node**: Fast mobile performance with full PoS support
- **Background Staking**: Earn 365%+ APR even when app is closed
- **Biometric Security**: Fingerprint/Face ID protection for all operations
- **Google Pay Integration**: Instant BabaChain purchases with one tap
- **Home Screen Widgets**: Real-time balance and staking information
- **Push Notifications**: Staking rewards, transactions, network alerts
- **No Server Dependency**: Direct blockchain connection, no downtime
- **Deterministic Wallet**: Single backup phrase restores everything
- **Private Key Security**: Keys never leave your device
- **Hardware Encryption**: Android Keystore protection
- **Auto-Compounding**: Staking rewards automatically increase your stake
- **Real-time Sync**: Live blockchain synchronization and updates

## Download

[![Get it on Google Play](https://play.google.com/intl/en_us/badges/static/images/badges/en_badge_web_generic.png)](https://play.google.com/store/apps/details?id=org.babachain.wallet)

*Coming Soon to Google Play Store*

## Project Structure

This project contains several sub-projects:

 * **wallet**: The Android app itself with mobile staking capabilities. [wallet README](wallet/README.md)
 * **common**: Contains common components used by integrations and features
 * **features**: Contains features such as Explore BabaChain and mobile staking
 * **integrations**: Various integrations: CrowdNode, Coinbase, Uphold
 * **integration-android**: Library for integrating BabaChain payments into your Android app
 * **sample-integration-android**: Example app demonstrating BabaChain payment integration
 * **market**: App description and promo material for the Google Play Store

## Getting Started

To run *BabaChain Wallet* Android app on your device or emulator clone the repo and make sure you have the required [development environment](#requirements).

### Quick Start for Users

1. **Download** BabaChain Wallet from Google Play Store
2. **Create** a new wallet or import existing seed phrase
3. **Enable** fingerprint/face unlock for security
4. **Buy** BabaChain instantly with Google Pay (optional)
5. **Start Staking** and earn 365%+ APR automatically!

### Quick Start for Developers

```bash
git clone https://github.com/baba-chain/babachain-android.git
cd babachain-android
./gradlew assembleProdRelease
```

The built APKs will be in `wallet/build/outputs/apk`

## Requirements

- Android 7.0 (API level 24) or later
- Android Studio Arctic Fox or later
- Gradle 7.0 or later
- Java 11 or later

### BabaChain Development Requirements

Currently, BabaChain wallet requires a few additional steps for development:

1. Clone [BabaChainJ](https://github.com/baba-chain/babachainj) repository:  
`git clone https://github.com/baba-chain/babachainj.git`  

2. Install required tools:
```bash
# On Ubuntu/Debian
sudo apt install openjdk-11-jdk android-tools-adb

# On macOS
brew install openjdk@11 android-platform-tools
```

3. Set up Android SDK and NDK:
```bash
export ANDROID_HOME=$HOME/Android/Sdk
export ANDROID_NDK_HOME=$ANDROID_HOME/ndk/21.4.7075529
```

4. Run `./gradlew build` in the wallet directory.

See [wallet/README.md](wallet/README.md) for more details.

## 🏗️ Architecture

### Core Components

- **LightNodeService**: Manages blockchain synchronization and PoS consensus
- **BackgroundStakingService**: Handles continuous staking and reward processing
- **BiometricAuthenticationManager**: Manages fingerprint/face unlock security
- **StakingNotificationService**: Handles push notifications and alerts
- **MobileWalletController**: Central coordinator for all mobile features
- **EnhancedQrScannerActivity**: Advanced QR code scanning for payments

### Security Architecture

- **Android Keystore**: Private keys stored in hardware security module
- **Biometric Protection**: All sensitive operations require biometric auth
- **Encrypted Storage**: All sensitive data encrypted at rest
- **Code Signing**: App integrity verification
- **App Sandboxing**: Isolated app environment

## 🔐 Security

### Mobile Security Features

- **Biometric Authentication**: Fingerprint, face unlock, and iris scanning support
- **Hardware Keystore**: Private keys stored in secure hardware
- **Auto-lock**: Configurable automatic wallet locking
- **Transaction Confirmation**: Biometric approval for all payments
- **Secure Backup**: Encrypted seed phrase storage

### Network Security

- **Direct Connection**: No intermediary servers or pools
- **Peer Verification**: Cryptographic validation of network peers
- **TLS Encryption**: All network communications encrypted
- **SPV Validation**: Lightweight but secure blockchain verification

## ⚠️ WARNING

Installation on rooted devices is strongly discouraged.

Any root app can grant itself access to every other app's data and rob you by accessing the Android Keystore or other sensitive information.

## 🌐 Community

- **Website**: https://www.babachain.org
- **Discord**: https://discord.gg/babachain
- **Twitter**: https://twitter.com/babachainorg
- **Telegram**: https://t.me/babachain
- **Reddit**: https://reddit.com/r/babachain

## Documentation

- **Official BabaChain Documentation**: [docs.babachain.org](https://docs.babachain.org)
- **Android Features Documentation**: [BABACHAIN_ANDROID_FEATURES.md](BABACHAIN_ANDROID_FEATURES.md)
- **Build Instructions**: [wallet/README.md](wallet/README.md)
- **Recovery Guide**: [wallet/README.recover.md](wallet/README.recover.md)

## License

*BabaChain Wallet* is available under the MIT license. See the LICENSE file for more info.

---

**Built with ❤️ by the BabaChain Community**

*Start earning 365%+ APR today with BabaChain's revolutionary mobile staking!*