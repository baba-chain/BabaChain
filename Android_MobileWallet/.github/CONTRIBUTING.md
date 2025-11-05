# Contributing Guidelines

We use Kotlin and Java for developing this revolutionary mobile staking wallet. Java is used on legacy components
and the underlying babachainj library. New features are written in Kotlin.

## 🚀 BabaChain Mobile Wallet Features

This wallet implements cutting-edge mobile staking technology:

- **365%+ APR Staking**: Revolutionary gradual bonus system
- **Background Staking**: Earn rewards 24/7 even when app is closed
- **Biometric Security**: Fingerprint and face unlock
- **Light Node (SPV)**: Efficient blockchain synchronization
- **Push Notifications**: Real-time staking rewards and network events
- **Enhanced QR Scanner**: Advanced payment processing

## 📋 Development Guidelines

General information on developing conventions can be found at [AOSP Java Code Style
for Contributors](https://source.android.com/setup/contribute/code-style).

While our code style is not enforced by git, please use the Code | Reformat Code
command in Android Studio.

### 🏗️ Architecture

- **MVVM Pattern**: Use ViewModel and LiveData for UI components
- **Dependency Injection**: Hilt for dependency management
- **Coroutines**: For asynchronous operations and background tasks
- **Room Database**: For local data persistence
- **WorkManager**: For background staking operations

### 🔒 Security Guidelines

- Always use biometric authentication for sensitive operations
- Store private keys in Android Keystore
- Encrypt all sensitive data at rest
- Use secure communication protocols

### 📱 Mobile-Specific Considerations

- Optimize for battery life in background services
- Handle network connectivity changes gracefully
- Implement proper lifecycle management for staking services
- Use efficient data structures for mobile performance

## 🐛 Issues and Discussions

If you've got a question or would like to start a discussion, please post to
[GitHub Issues](https://github.com/baba-chain/babachain-android/issues).

## 🌍 Translations

If you would like to contribute language translations, we prefer if you use our
[Transifex project](https://www.transifex.com/babachain/babachain-mobile-wallets/). Languages will be
synced from there regularly.

## 🎯 Focus Areas for Contributors

We're particularly interested in contributions to:

1. **Staking Optimization**: Improving reward calculation algorithms
2. **Battery Efficiency**: Optimizing background services
3. **Security Enhancements**: Additional biometric features
4. **UI/UX Improvements**: Better user experience for staking
5. **Performance**: Mobile-specific optimizations
6. **Testing**: Unit and integration tests for staking features

---

**Help us build the future of mobile cryptocurrency staking!**
