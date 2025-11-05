# Build Issues and Solutions

## Prerequisites

Check that you have **BabaChainSync** repo next to the wallet repo.

Make sure you're on the **master** branch for both BabaChainSync and BabaChainWallet.

### Clang Version Check

Check clang version:

```bash
clang++ --version
```

If you see homebrew version, this might cause issues with building. You need to switch to system version.

```
❌ InstalledDir: /opt/homebrew/opt/llvm/bin
✅ InstalledDir: /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin
```

To unplug homebrew version, comment out the following in `~/.zshrc` or `~/.zsh_profile` (or `~/.bashrc` if you're using bash):

```bash
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
```

## Required Dependencies

Install the following dependencies:

- **cmake** (version 3.28.3 recommended)
- **cbindgen**
- **rust**

> **Note:** cmake 4.0.0 or higher might cause issues with building. Version 3.28.3 is tested and works.
> 
> Download from: https://github.com/Kitware/CMake/releases/download/v3.28.3/cmake-3.28.3-macos-universal.dmg

### Installation Commands

```bash
# Install CMake
sudo "/Applications/CMake.app/Contents/bin/cmake-gui" --install

# Install cbindgen
brew install cbindgen

# Install Rust
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

### Xcode Setup

```bash
xcode-select --install
sudo xcodebuild -license accept
```

### Pod Installation

Run from the wallet directory:

```bash
pod install --verbose
```

## Potential Issues and Solutions

### Issue 1: CBind Generation Error

**Error:**
```
Compiling rs-merk-verify-c-binding v0.1.3 (https://github.com/dashpay/rs-merk-verify-c-binding?branch=for-use-in-main-crate#930aeb2a)
error: failed to run custom build command for `dash-spv-coinjoin v0.1.0 (/Users/username/Development/dash-shared-core/dash-spv-coinjoin)`
```

**Solution:** Install cbindgen if not already installed:

```bash
brew install cbindgen
```

---

### Issue 2: Git Repository Error with BabaChain-GRPC

**Error:**
```
Installing BabaChain-GRPC (1.0.0)
 > Git download
     $ /usr/bin/git clone https://github.com/baba-chain/babachainsync-iOS.git
     ...
   fatal: not a git repository (or any of the parent directories): .git
```

**Solution:** Downgrade CocoaPods to version 1.15.2. This might require upgrading Ruby to version 3.3.0 or higher:

```bash
# Install Ruby
brew install ruby

# Install specific CocoaPods version
sudo gem install cocoapods -v 1.15.2
```

**Alternative installation:**
```bash
sudo /opt/homebrew/opt/ruby/bin/gem install -n /usr/local/bin cocoapods -v 1.15.2
```

---

### Issue 3: 'babachain_shared_core.h' file not found

**Problem:** babachain-shared-core was not compiled properly due to various potential reasons.

**Solution:** Check the build log for more details. Usually caused by missing dependencies (see above).

## babachain-shared-core Development

If you want to make modifications to babachain-shared-core, follow these steps:

### Setup Steps

1. **Place babachain-shared-core in the correct location:**
   Put babachain-shared-core in the same directory as the wallet repo.

2. **Remove BabaChainSharedCore from BabaChainSync.podspec:**
   ```ruby
   # s.dependency 'BabaChainSharedCore', '1.0.0'
   ```

3. **Add local babachain-shared-core to Podfile:**
   In the wallet or BabaChainSync example Podfile, add:
   ```ruby
   pod 'BabaChainSharedCore', :path => '../babachain-shared-core/'
   ```

### Issue 4: Bad CPU type in executable
The protoc compiler that is part of babachain-grpc-pod-installer is for intel chips. You will need to have Rosetta installed
to allow this protoc compiler to run.

```
> FETCH_HEAD /bin/bash: line 10: babachain-grpc-pod-installer/Pods/!ProtoCompiler/protoc: Bad CPU type in executable
```
```bash
softwareupdate --install-rosetta --agree-to-license
pod deintegrate

# uninstall cocoapods 
sudo gem uninstall cocoapods 

# Install CocoaPods natively for Apple Silicon, but the correct version 1.15.2 
sudo arch -arm64 gem install cocoapods -v 1.15.2

pod install --verbose
```