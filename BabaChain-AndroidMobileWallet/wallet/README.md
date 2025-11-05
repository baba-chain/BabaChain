# BabaChain Android Wallet

BabaChain Wallet is a revolutionary mobile staking wallet for the BabaChain Proof-of-Stake cryptocurrency. 
This repo contains the source code for the Android platform. iOS is supported 
at the [babachain-ios](https://github.com/baba-chain/babachain-ios) repo on Github.

[![Build Status](https://github.com/baba-chain/babachain-android/actions/workflows/android.yml/badge.svg)](https://github.com/baba-chain/babachain-android/actions)
[![License](https://img.shields.io/badge/license-MIT-green)](https://github.com/baba-chain/babachain-android/blob/master/LICENSE)
[![Platform](https://img.shields.io/badge/platform-Android-green)](https://github.com/baba-chain/babachain-android)

*BabaChain Wallet* connects directly to the BabaChain network using SPV light node technology for fast mobile performance while maintaining full security. Built with Android-native features like biometric authentication, background services, and hardware security integration.

## 🚀 Revolutionary Features

### 💰 Mobile Staking with 365%+ APR
- **Background Staking**: Earn rewards 24/7 even when app is closed
- **Gradual Bonus System**: 1% daily base + up to 20% bonus for larger stakes
- **No Minimum**: Stake any amount (even 1 BABA works!)
- **Auto-Compounding**: Rewards automatically increase your stake
- **Real-time Notifications**: Get alerted when you earn rewards

### 🔒 Advanced Security
- **Biometric Authentication**: Fingerprint and face unlock
- **Hardware Security**: Private keys stored in Android Keystore
- **Transaction Protection**: Biometric confirmation for all payments
- **Encrypted Backups**: Secure wallet backup and recovery

### 📱 Android-Specific Features
- **Home Screen Widgets**: Real-time balance and staking info
- **Background Services**: Continuous staking and sync
- **Push Notifications**: Staking rewards, transactions, network status
- **NFC Support**: Tap-to-pay and receive functionality
- **Offline Mode**: View balance and history without internet

## 📊 BabaChain Economics

| Parameter | Value |
|-----------|-------|
| **Initial Supply** | 210,000,000 BABA |
| **Premine** | 20,000,000 BABA (~10% - fair distribution) |
| **Community Rewards** | 190,000,000 BABA (~90% for stakers) |
| **Base Staking Rate** | 1% daily (365% APR) |
| **Gradual Bonuses** | Up to 20% additional for larger stakes |

Technical details
=================

### FILES

Your wallet contains your private keys and various transaction related metadata. It is stored in app-private
storage:

	Mainnet: /data/data/org.babachain.wallet/files/wallet-protobuf (MODE_PRIVATE)
	Testnet: /data/data/org.babachain.wallet_test/files/wallet-protobuf-testnet (MODE_WORLD_READABLE | MODE_WORLD_WRITEABLE)

The wallet file format is not compatible to wallet.dat (Bitcoin client). Rather, it uses a custom protobuf format
which should be compatible between clients using babachainj.

Certain actions cause automatic rolling backups of your wallet to app-private storage:

	Mainnet: /data/data/org.babachain.wallet/files/key-backup-protobuf (MODE_PRIVATE)
	Testnet: /data/data/org.babachain.wallet_test/files/key-backup-protobuf-testnet (MODE_PRIVATE)

Your wallet can be manually backed up to and restored from external storage:

	Mainnet: /sdcard/Download/babachain-wallet-backup-<yyyy-MM-dd>
	Testnet: /sdcard/Download/babachain-wallet-backup-testnet-<yyyy-MM-dd>

Your wallet can be manually backed up and restored using a recovery phrase (12 word mnemonic).

If you want to recover coins from manual backups and for whatever reason you cannot use the app
itself to restore from the backup, see the separate [README.recover.md](README.recover.md) guide.

The current fee rate for each of the fee categories (economic, normal, priority) is cached in
app-private storage:

    Mainnet: /data/data/org.babachain.wallet/files/fees.txt
    Testnet: /data/data/org.babachain.wallet_test/files/fees-testnet.txt

### STAKING DATA

BabaChain Wallet stores staking-related data in app-private storage:

    Mainnet: /data/data/org.babachain.wallet/files/staking-data
    Testnet: /data/data/org.babachain.wallet_test/files/staking-data-testnet

This includes:
- Staking status and configuration
- Reward history and statistics
- Validator information
- Background staking state

### DEBUGGING

Wallet file for Testnet can be pulled from an (even un-rooted) device using:

	adb pull /data/data/org.babachain.wallet/files/wallet-protobuf-testnet

Log messages can be viewed by:

    adb logcat

The app can send extensive debug information. Use **Options > Settings > Report Issue** and follow the dialog.
In the generated e-mail, replace the support address with yours.

## Quick Start

### For Users
1. **Download** BabaChain Wallet from Google Play Store
2. **Create** a new wallet or import existing seed phrase
3. **Enable** biometric authentication for security
4. **Start Staking** and earn 365%+ APR automatically!

### For Developers

#### Prerequisites
- Android Studio Arctic Fox or later
- Java 11 or later
- Android SDK with API level 24+
- Android NDK (for native cryptographic operations)

#### Development Setup

```bash
# Clone the repository
git clone https://github.com/baba-chain/babachain-android.git
cd babachain-android

# Install dependencies (Ubuntu/Debian)
sudo apt install git gradle openjdk-11-jdk android-tools-adb

# Set environment variables
export ANDROID_HOME=$HOME/Android/Sdk
export ANDROID_NDK_HOME=$ANDROID_HOME/ndk/21.4.7075529

# Build debug version
./gradlew assembleTestNet3Debug

# Install on device
adb install wallet/build/outputs/apk/wallet-_testNet3-debug.apk
```

### BUILDING THE DEVELOPMENT VERSION (TESTNET)

The development version uses Testnet for safe testing and debugging.

**Build Flavors:**
- `_testNet3`: TestNet build for development
- `staging`: Staging environment build
- `devnet`: Development network build
- `prod`: Production mainnet build

**Development Build:**
```bash
./gradlew clean assemble_testNet3Debug -x test
```

**Production Build:**
```bash
./gradlew clean assembleProdRelease
```

## 🏗️ Architecture

### Core Components

- **LightNodeService**: SPV blockchain synchronization and PoS consensus
- **BackgroundStakingService**: Continuous staking with WorkManager integration
- **BiometricAuthenticationManager**: Hardware-backed biometric security
- **StakingNotificationService**: Push notifications for rewards and events
- **EnhancedQrScannerActivity**: Advanced QR code scanning with Camera2 API
- **MobileWalletController**: Central coordinator for all mobile features

### Security Architecture

- **Android Keystore**: Hardware-backed private key storage
- **Biometric Protection**: Fingerprint/face authentication for all operations
- **Encrypted Storage**: All sensitive data encrypted at rest
- **Foreground Services**: Secure background operation management

### Dependencies

#### BabaChain Core Dependencies
```bash
# Clone required repositories
git clone https://github.com/baba-chain/babachainj.git
git clone https://github.com/baba-chain/android-babachainj.git
git clone https://github.com/baba-chain/babachain-client-android.git
git clone https://github.com/baba-chain/android-babachainpay.git

# Build dependencies
cd babachainj && ./gradlew assemble
cd ../android-babachainj && ./gradlew build
cd ../babachain-client-android && ./gradlew build
cd ../android-babachainpay && ./gradlew build
```

#### Android-Specific Libraries
- **AndroidX Biometric**: Biometric authentication
- **WorkManager**: Background task scheduling
- **Room Database**: Local data persistence
- **Camera2 API**: Enhanced QR code scanning
- **Firebase Messaging**: Push notifications

### BUILDING THE PRODUCTION VERSION

These files must exist to result in a fully functional build:
* `wallet/google-services.json` - supports analytics, crash-lytics, google cloud services, etc
* `services.properties` - contains the keys for Uphold and Coinbase (see below)
* `local.properties` - contains the support email and Google Map API keys (see below)

At this point I'd like to remind that you continue on your own risk. According to the license,
there is basically no warranty and liability. It's your responsibility to audit the source code
for security issues and build, install and run the application in a secure way.

The production version uses Mainnet, is built non-debuggable, space-optimized with ProGuard and the
wallet file is protected against access from non-root users. In the code repository, it is build with
the 'prod' flavor.

	# each time
	cd babachain-wallet
	git pull
    gradle clean build assembleProdRelease

The resulting production release version will be at:  `wallet/build/outputs/apk/wallet-prod-release.apk`

BUILDING ALL FLAVORS

	# each time
	cd babachain-wallet
	git pull
    gradle clean build assembleProdRelease
    gradle clean build assemble_testNet3Release
    gradle clean build assembleProdDebug
    gradle clean build assemble_testNet3Debug
    
All flavors (debug and release) will be at:  wallet/build/outputs/apk

### BUILDING THE PRODUCTION VERSION WITH FASTLANE
Place these files in `./deploy`
* `app-distribution-key.json` - Firebase app distribution key
* `babachain-wallet.keystore` - the production signing key
* `gc-storage-service-account.json` - Google Cloud Storage key

## Build the production version of the app
The APK is placed here: `wallet/build/outputs/apk/wallet-prod-release.apk`
```sh
    fastlane build storepass:[keystore password] keypass:[key password]
```
## Upload existing wallet-prod-release.apk file to the internal test track.
```shell
    fastlane upload
```
## Build, followed by Upload
```shell
    fastlane publish storepass:[keystore password] keypass:[key password]
```
## Promote to production with 50% rollout (20% by default if no args)
```shell
    fastlane promote rollout:0.5
```
## Increase rollout to 70%
```shell
    fastlane increase rollout:0.7
```

### CONFIGURATION FOR UPHOLD AND COINBASE

The file services.properties must be in the root folder of the repo with the keys for the Uphold and
Coinbase Services as follows:

```
UPHOLD_CLIENT_ID="<uphold client id>"
UPHOLD_CLIENT_SECRET="<uphold secret>"
UPHOLD_CLIENT_ID_SANDBOX="<uphold sandbox client id>"
UPHOLD_CLIENT_SECRET_SANDBOX="<uphold sandbox secret"
COINBASE_CLIENT_ID="<coinbase client id>"
COINBASE_CLIENT_SECRET="<coinbase secret>"
```

### CONFIGURATION FOR SUPPORT EMAIL

The default support email used by BabaChain Wallet will be an empty string. However, this can be 
customized. `build.gradle` will assign a value `BuildConfig.SUPPORT_EMAIL` will be assigned 
according to the following:

The email will be determined by looking in `local.properties` followed by the environment for these
two variables:
- SUPPORT_EMAIL
- INTERNAL_SUPPORT_EMAIL - if the build is debug or SUPPORT_EMAIL is empty, then this will be used.

This allows `local.properties` to specify a support email for debug builds and a different support 
email for release/production builds. 

### CONFIGURATION FOR GOOGLE MAPS

`local.properties` should also have a value for `GOOGLE_PLAY_API_KEY` to support Google Maps in the
Explore features of BabaChain Wallet.

### SETTING UP FOR DEVELOPMENT

You should be able to import the project into Android Studio, as it uses Gradle for building.
* Set the build variant on the wallet module to the required flavor (mobileDebug for mobile devnet)
* From Tools | SDK Manager, select Android SDK Build Tools version 30 and NDK (side by side) version 21 or above

### TRANSLATIONS

The source language is English. Translations for all other languages [happen on Transifex](https://app.transifex.com/babachain/babachain-mobile-wallets/).
The source text and translations are shared as much as possible with the iOS app.

The English resources are pushed to Transifex. Changes are pulled and committed to the git
repository from time to time. It can be done by manually downloading the files, but using the `tx`
command line client is more convenient. See [Transifex Client](https://developers.transifex.com/docs/cli)
for help for usage and installation.

If strings resources are added or changed, the source language files need to be pushed to
Transifex. This step will probably only be executed by the maintainer of the project, as special
permission is needed:

    # push source files to Transifex
    tx push -s

As soon as a translation is ready, it can be pulled:

    # pull translation from Transifex
    tx pull -f -l <language code>

    # pull all translations > 50% complete from Transifex
    tx pull -f --minimum-perc=50

Note that after pulling, any bugs introduced by either translators or Transifex itself need to be
corrected manually.

## 📱 Android-Specific Features

### NFC (Near Field Communication)

BabaChain Wallet supports reading BabaChain requests via NFC, either from a passive NFC tag or from
another NFC capable Android device that is requesting coins.

**Features:**
- **Tap-to-Pay**: Send payments by tapping NFC-enabled devices
- **Tap-to-Receive**: Request payments via NFC
- **NFC Tag Support**: Read payment requests from NFC tags
- **Peer-to-Peer**: Direct NFC payments between Android devices

**Usage:**
1. Enable NFC in your Android device settings
2. Hold your phone to the NFC tag or device
3. The payment dialog will open with pre-filled information

**NFC Tag Setup:**
- Use [NFC TagWriter](https://play.google.com/store/apps/details?id=com.nxp.nfc.tagwriter) or similar apps
- Minimum 1 KB capacity tags recommended
- Format: `babachain:BabaChainAddressExample123456789?amount=1.5&label=Payment`
- Set message type to URI or URL (not Text)
- Enable write protection for public tags

### Home Screen Widgets

BabaChain Wallet provides Android home screen widgets for quick access to wallet information:

**Widget Types:**
- **Balance Widget**: Shows current BABA balance and USD value
- **Staking Widget**: Displays staking status and recent rewards
- **Quick Actions Widget**: Fast access to send, receive, and staking

**Widget Features:**
- Real-time balance updates
- Staking reward notifications
- One-tap access to common actions
- Customizable refresh intervals
- Dark/light theme support

### Background Services

**Foreground Services:**
- **LightNodeService**: Continuous blockchain synchronization
- **BackgroundStakingService**: 24/7 staking operation

**WorkManager Tasks:**
- **StakingWorker**: Periodic staking checks every 15 minutes
- **SyncWorker**: Blockchain synchronization tasks
- **NotificationWorker**: Push notification processing

**Battery Optimization:**
- Doze mode compatibility
- App standby handling
- Intelligent task scheduling
- Network-aware operations

### BABACHAINJ

BabaChain Wallet uses [babachainj](https://github.com/baba-chain/babachainj) for BabaChain specific logic. This project is forked from [bitcoinj](https://bitcoinj.github.io/) with PoS consensus support.

## URL Schemes

BabaChain Wallet supports the following URL schemes for Android integration:

- `babachain://` - Standard BabaChain payments
- `babachainwallet://` - Wallet-specific actions
- `babachainid://` - BabaChain identity operations

**Example Usage:**
```
babachain:BabaChainAddressExample123456789?amount=1.5&label=Payment
babachainwallet://staking/start
babachainid://profile/view?username=alice
```

### EXCHANGE RATES

BabaChain Wallet has multiple sources for exchange rates:
- Source 1: CTX - https://rates.ctx.com/rates?source=ctx (BabaChain rates)
- Source 2: CoinGecko API for BabaChain price data
- Source 3: CoinMarketCap API for additional price feeds
- Source 4: BitPay (BTC/all), BabaChain Central(BABA/BTC), Poloniex (BABA/BTC)

### SWEEPING WALLETS

When sweeping wallets, BabaChain Wallet uses a set of Electrum servers and block explorers to query for
unspent transaction outputs (UTXOs).

### STAKING IMPLEMENTATION

BabaChain Wallet implements Proof-of-Stake consensus with the following features:

#### Staking Process
1. **Coin Maturity**: Coins must be held for 8 hours before they can stake
2. **Staking Weight**: Calculated based on coin age and amount
3. **Block Creation**: Wallet participates in block validation and creation
4. **Reward Distribution**: Automatic reward distribution based on gradual bonus system

#### Background Staking
- **Service Architecture**: Uses Android foreground services for continuous operation
- **Battery Optimization**: Intelligent scheduling to minimize battery drain
- **Network Efficiency**: Optimized network usage for mobile devices
- **Notification System**: Real-time notifications for staking events

#### Security Features
- **Private Key Protection**: Keys never leave the device
- **Biometric Authentication**: Required for staking operations
- **Hardware Security**: Integration with Android Keystore
- **Encrypted Storage**: All staking data is encrypted at rest

## 🚀 Deployment

### Google Play Store

The production version is available on Google Play Store:

[![Get it on Google Play](https://play.google.com/intl/en_us/badges/static/images/badges/en_badge_web_generic.png)](https://play.google.com/store/apps/details?id=org.babachain.wallet)

### APK Distribution

Direct APK downloads are available from GitHub releases:
- **Mainnet**: `wallet-prod-release.apk`
- **Testnet**: `wallet-_testNet3-release.apk`

### System Requirements

- **Android Version**: 7.0 (API level 24) or later
- **RAM**: Minimum 2GB, recommended 4GB+
- **Storage**: 500MB free space for blockchain data
- **Network**: Internet connection for initial sync
- **Hardware**: Biometric sensor (recommended)

## ⚠️ Security Warning

**Rooted Devices**: Installation on rooted devices is strongly discouraged. Root access can compromise the Android Keystore and expose private keys to malicious applications.

**Unknown Sources**: Only install BabaChain Wallet from Google Play Store or official GitHub releases. Avoid third-party app stores or unofficial sources.

## 🌐 Community & Support

- **Website**: https://www.babachain.org
- **Documentation**: https://docs.babachain.org
- **Discord**: https://discord.gg/babachain
- **Twitter**: https://twitter.com/babachainorg
- **Telegram**: https://t.me/babachain
- **Reddit**: https://reddit.com/r/babachain
- **GitHub**: https://github.com/baba-chain

### Getting Help

1. **Documentation**: Check [docs.babachain.org](https://docs.babachain.org) for guides
2. **GitHub Issues**: Report bugs and request features
3. **Discord**: Join our community for real-time support
4. **Email**: Contact support@babachain.org for critical issues

## 📄 License

BabaChain Wallet is available under the MIT license. See the [LICENSE](../LICENSE) file for more info.

---

**Built with ❤️ by the BabaChain Community**

*Start earning 365%+ APR today with BabaChain's revolutionary mobile staking!*