Welcome to _BabaChain Wallet_, a revolutionary Android app for earning and managing BabaChain with 365%+ APR staking!

This project contains several sub-projects:

 * __wallet__:
     The Android app itself with mobile staking capabilities. This is probably what you're searching for. [wallet README](wallet/README.md)
 * __common__:
     Contains common components used by integrations.
 * __features__:
     Contains features such as Explore BabaChain and mobile staking
 * __market__:
     App description and promo material for the Google Play app store.
 * __integration-android__:
      A tiny library for integrating BabaChain payments into your own Android app
     (e.g. donations, in-app purchases, staking rewards).
 * __integrations__
     Contains the various integrations: CrowdNode, Coinbase, Uphold, Apple Pay
 * __sample-integration-android__:
     A minimal example app to demonstrate integration of digital payments and staking into
     your Android app.

You can build the production version using Gradle:

`./gradlew assembleProdRelease`

The built apks will be in `wallet/build/outputs/apk`

## 🚀 Revolutionary Mobile Staking Features

### 💰 Earn 365%+ APR with Mobile Staking
- **Background Staking**: Earn rewards 24/7 even when app is closed
- **Gradual Bonus System**: 1% daily base + up to 20% bonus for larger stakes
- **No Minimum**: Stake any amount (even 1 BABA works!)
- **Auto-Compounding**: Rewards automatically increase your stake
- **Real-time Notifications**: Get alerted when you earn rewards

### 🔒 Advanced Security
- **Biometric Authentication**: Fingerprint and face unlock
- **Hardware Security**: Private keys stored in secure hardware
- **Transaction Protection**: Biometric confirmation for all payments
- **Encrypted Backups**: Secure wallet backup and recovery

### 📱 Android-Specific Features
- **Home Screen Widgets**: Real-time balance and staking info
- **Background Services**: Continuous staking and sync
- **Push Notifications**: Staking rewards, transactions, network status
- **NFC Support**: Tap-to-pay and receive functionality
- **Offline Mode**: View balance and history without internet

## 📊 BabaChain Economics

BabaChain features fair and sustainable economics:

| Parameter | Value |
|-----------|-------|
| **Initial Supply** | 210,000,000 BABA |
| **Maximum Supply** | 1,000,000,000 BABA (hard cap) |
| **Premine** | 20,000,000 BABA (~10% - fair distribution) |
| **Community Rewards** | 190,000,000 BABA (~90% for stakers) |
| **Base Staking Rate** | 1% daily (365% APR) |
| **Gradual Bonuses** | Up to 20% additional for larger stakes |

### 🎯 Gradual Bonus System

The Android wallet calculates your exact rewards using BabaChain's innovative gradual bonus system:

- **1-10,000 BABA**: 0% to 5% gradual bonus
- **10,000-100,000 BABA**: 5% to 20% gradual bonus  
- **100,000+ BABA**: Maximum 20% bonus

**Example**: 15,000 BABA stake earns 1.056% daily = **385% APR**

---

**Start earning 365%+ APR today with BabaChain's revolutionary mobile staking!**