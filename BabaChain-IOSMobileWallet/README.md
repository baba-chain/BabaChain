# BabaChain Wallet

[![Build Status](https://github.com/baba-chain/babachain-ios/actions/workflows/semantic-pull-request.yml/badge.svg)](https://github.com/baba-chain/babachain-ios/actions) [![License](https://img.shields.io/badge/license-MIT-green)](https://github.com/baba-chain/babachain-ios/blob/master/LICENSE) [![Release](https://img.shields.io/github/v/release/baba-chain/babachain-ios)](https://github.com/baba-chain/babachain-ios/releases) [![Platform](https://img.shields.io/badge/platform-iOS%20%7C%20watchOS-blue)](https://github.com/baba-chain/babachain-ios)

<p align="center" >
<img src="https://www.babachain.org/images/babachain_logo.png" alt="BabaChain Wallet logo" title="BabaChain Wallet" width="300">
</p>

*BabaChain Wallet* is a next-generation mobile wallet for the [BabaChain](https://babachain.org) Proof-of-Stake blockchain. There is no server to get hacked or go down, so you can always access your money and earn staking rewards.
Using [SPV](https://en.bitcoin.it/wiki/Thin_Client_Security#Header-Only_Clients) light node technology, *BabaChain Wallet* connects directly to the BabaChain network with the fast performance you need on a mobile device.
*BabaChain Wallet* is designed to protect you from malware, browser security holes, even physical theft. With Face ID/Touch ID biometric authentication, hardware-backed keychain storage, app sandboxing,
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
- **Face ID/Touch ID**: Biometric authentication for all operations
- **Hardware Keychain**: Private keys stored in secure enclave
- **Transaction Protection**: Biometric confirmation for all payments
- **Seed Phrase Security**: Encrypted backup with biometric access
- **Auto-lock**: Automatic wallet locking for security

### 📱 iOS-Specific Features
- **Home Screen Widgets**: Real-time balance and staking info
- **Apple Pay Integration**: Buy BabaChain instantly with one tap
- **Push Notifications**: Staking rewards, transactions, network status
- **Background App Refresh**: Continuous staking and sync
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
- **Biometric Security**: Face ID/Touch ID protection for all operations
- **Apple Pay Integration**: Instant BabaChain purchases with one tap
- **Home Screen Widgets**: Real-time balance and staking information
- **Push Notifications**: Staking rewards, transactions, network alerts
- **No Server Dependency**: Direct blockchain connection, no downtime
- **Deterministic Wallet**: Single backup phrase restores everything
- **Private Key Security**: Keys never leave your device
- **Hardware Encryption**: Secure enclave protection
- **Auto-Compounding**: Staking rewards automatically increase your stake
- **Real-time Sync**: Live blockchain synchronization and updates

## Download

[![Download on the AppStore](https://linkmaker.itunes.apple.com/en-gb/badge-lrg.svg?releaseDate=2024-01-01&kind=iossoftware&bubble=ios_apps)](https://apps.apple.com/app/babachain-wallet/id1234567890?mt=8)

*Coming Soon to the App Store*

## Getting Started

To run *BabaChain Wallet* iOS app on your device or simulator clone the repo and make sure you installed needed [Requirements](#Requirements).
Then run `pod install` in the cloned directory.
Open `BabaChainWallet.xcworkspace` in Xcode and run the project.

### Quick Start for Users

1. **Download** BabaChain Wallet from the App Store
2. **Create** a new wallet or import existing seed phrase
3. **Enable** Face ID/Touch ID for security
4. **Buy** BabaChain instantly with Apple Pay (optional)
5. **Start Staking** and earn 365%+ APR automatically!

## Requirements

- iOS 14.0 or later
- Xcode 13 or later
- Dependency manager [CocoaPods](https://cocoapods.org). Install via `gem install cocoapods`

### BabaChain Development Requirements

Currently, BabaChain wallet requires a few additional steps for development:

1. Clone [BabaChainSync](https://github.com/baba-chain/babachainsync-iOS) repository:  
`git clone https://github.com/baba-chain/babachainsync-iOS.git BabaChainSync`  

To simplify developing process we use local podspec dependencies and it's important to preserve the following folder structure:
```
../BabaChainSync/
../babachain-ios/
```

2. Install required tools:
```bash
brew install protobuf grpc cmake
```

3. Install Rust (for cryptographic operations):
```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

4. Run `pod install` in the wallet directory.

See [BUILD.md](BUILD.md) for more details.

### Optional Requirements

#### Objective-C Related
- Formatting tools: [clang-format](https://clang.llvm.org/docs/ClangFormat.html). Install via `brew install clang-format`.

#### Swift Related
- [SwiftFormat](https://github.com/nicklockwood/SwiftFormat). Install via `brew install swiftformat`. 
- [SwiftLint](https://github.com/realm/SwiftLint).  Install via `brew install swiftlint`.

#### Localization
- Localized files helper tool [BartyCrouch](https://github.com/Flinesoft/BartyCrouch). Install via `brew install bartycrouch`.

## 🏗️ Architecture

### Core Components

- **SPVLightNode**: Manages blockchain synchronization and PoS consensus
- **BackgroundStakingManager**: Handles continuous staking and reward processing
- **BiometricAuthentication**: Manages Face ID/Touch ID security
- **BabaChainNotificationManager**: Handles push notifications and alerts
- **ApplePayIntegration**: Manages instant BabaChain purchases
- **Widget Extension**: Provides home screen widgets

### Security Architecture

- **Hardware Security Module**: Private keys stored in secure enclave
- **Biometric Protection**: All sensitive operations require Face ID/Touch ID
- **Keychain Integration**: Encrypted storage with hardware backing
- **Code Signing**: App integrity verification
- **Sandboxing**: Isolated app environment

## Contribution Guidelines

We use Objective-C for developing iOS App and underlying [BabaChainSync](https://github.com/baba-chain/babachainsync-iOS) library and Swift for modern features like widgets and staking UI.

General information on developing conventions you can find at [Apple Developer Portal](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/ProgrammingWithObjectiveC/Conventions/Conventions.html).
For more specific Objective-C guidelines we stick with [NYTimes Objective-C Style Guide](https://github.com/nytimes/objective-c-style-guide).

Our code style is enforced by [clang-format](#Objective-C-Related) and [SwiftFormat / SwiftLint](#Swift-Related).

## Documentation

- **Official BabaChain Documentation**: [docs.babachain.org](https://docs.babachain.org)
- **iOS Features Documentation**: [BABACHAIN_IOS_FEATURES.md](BABACHAIN_IOS_FEATURES.md)
- **Build Instructions**: [BUILD.md](BUILD.md)
- **Premine Updates**: [PREMINE_UPDATES_SUMMARY.md](PREMINE_UPDATES_SUMMARY.md)

## URL Schemes

BabaChain Wallet supports the following URL schemes:

- `babachain://` - Standard BabaChain payments
- `babachainwallet://` - Wallet-specific actions
- `babachainid://` - BabaChain identity operations

For more information follow the [BabaChain URL Scheme documentation](https://docs.babachain.org/wallets/ios/url-schemes.html).

## 🔐 Security

### Mobile Security Features

- **Biometric Authentication**: Face ID, Touch ID, and Optic ID support
- **Hardware Keychain**: Private keys stored in secure enclave
- **Auto-lock**: Configurable automatic wallet locking
- **Transaction Confirmation**: Biometric approval for all payments
- **Secure Backup**: Encrypted seed phrase storage

### Network Security

- **Direct Connection**: No intermediary servers or pools
- **Peer Verification**: Cryptographic validation of network peers
- **TLS Encryption**: All network communications encrypted
- **SPV Validation**: Lightweight but secure blockchain verification

## ⚠️ WARNING

Installation on jailbroken devices is strongly discouraged.

Any jailbreak app can grant itself access to every other app's keychain data and rob you by self-signing as described [here](http://www.saurik.com/id/8) and including `<key>application-identifier</key><string>*</string>` in its .entitlements file.

## 🌐 Community

- **Website**: https://www.babachain.org
- **Discord**: https://discord.gg/babachain
- **Twitter**: https://twitter.com/babachainorg
- **Telegram**: https://t.me/babachain
- **Reddit**: https://reddit.com/r/babachain

## License

*BabaChain Wallet* is available under the MIT license. See the LICENSE file for more info.

---

**Built with ❤️ by the BabaChain Community**

*Start earning 365%+ APR today with BabaChain's revolutionary mobile staking!*