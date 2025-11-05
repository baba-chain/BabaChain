# Turkish Language Support in BabaChain

This document describes the comprehensive Turkish language support implemented in BabaChain Core.

## Overview

BabaChain Core includes full Turkish language support across all platforms:
- Desktop Qt Wallet
- Android Mobile Wallet  
- iOS Mobile Wallet
- Core daemon and RPC interface
- Database storage
- Logging system

## Features

### 1. Desktop Wallet (Qt)

#### Turkish Translation
- Complete Turkish translation file (`src/qt/locale/babachain_tr.ts`)
- Includes all UI elements, menus, dialogs, and messages
- Staking-related terminology properly translated
- Error messages and help text in Turkish

#### Turkish Locale Support
- Automatic Turkish locale detection and configuration
- Turkish number formatting (comma as decimal separator)
- Proper handling of Turkish characters (ç, ğ, ı, ö, ş, ü, Ç, Ğ, İ, Ö, Ş, Ü)
- UTF-8 encoding throughout the application

#### Usage
To use the Turkish interface:
1. Set system locale to Turkish (tr_TR)
2. Or use command line: `babachaind -lang=tr`
3. Or set in GUI: Settings → Options → Display → Language

### 2. Mobile Wallets

#### Android Wallet
- Turkish string resources in `BabaChain-AndroidMobileWallet/wallet/res/values-tr/`
- Complete translation of all UI elements
- Staking functionality translated
- Turkish character support in all input fields
- Proper UTF-8 encoding

#### iOS Wallet  
- Turkish localization in `BabaChain-IOSMobileWallet/BabaChainWallet/tr.lproj/`
- Native iOS Turkish language support
- Staking interface translated
- Turkish keyboard layout support
- Voice-over support for accessibility

### 3. Core Daemon and RPC

#### Character Encoding Validation
- UTF-8 validation for all RPC parameters containing Turkish characters
- Proper error handling for invalid character encoding
- Turkish character support in wallet labels and transaction descriptions

#### Database Support
- UTF-8 encoding in LevelDB storage
- Turkish character support in all stored data
- Proper collation for Turkish text sorting

### 4. Logging System

#### Turkish Character Support
- UTF-8 encoding in log files
- Turkish character validation in log messages
- Proper handling of Turkish text in error messages

## Technical Implementation

### Character Encoding
All Turkish characters are properly handled using UTF-8 encoding:
- ç (U+00E7) - Latin Small Letter C with Cedilla
- Ç (U+00C7) - Latin Capital Letter C with Cedilla  
- ğ (U+011F) - Latin Small Letter G with Breve
- Ğ (U+011E) - Latin Capital Letter G with Breve
- ı (U+0131) - Latin Small Letter Dotless I
- İ (U+0130) - Latin Capital Letter I with Dot Above
- ö (U+00F6) - Latin Small Letter O with Diaeresis
- Ö (U+00D6) - Latin Capital Letter O with Diaeresis
- ş (U+015F) - Latin Small Letter S with Cedilla
- Ş (U+015E) - Latin Capital Letter S with Cedilla
- ü (U+00FC) - Latin Small Letter U with Diaeresis
- Ü (U+00DC) - Latin Capital Letter U with Diaeresis

### Turkish Utility Functions
Located in `src/util/turkish.h` and `src/util/turkish.cpp`:

- `ContainsTurkishChars()` - Detect Turkish characters in strings
- `IsValidUTF8()` - Validate UTF-8 encoding
- `TurkishToASCII()` - Convert Turkish characters to ASCII equivalents
- `NormalizeTurkish()` - Normalize Turkish strings for comparison
- `FormatAmountTurkish()` - Format amounts with Turkish locale rules

### Number Formatting
Turkish locale uses:
- Comma (,) as decimal separator
- Thin space as thousands separator
- Example: 1 234 567,89 BABACHAIN

## Testing

### Unit Tests
Run Turkish character tests:
```bash
./test_babachain --run_test=turkish_tests
```

### Functional Tests
Run Turkish validation tests:
```bash
./test/functional/turkish_validation.py
```

### Manual Testing Checklist

#### Desktop Wallet
- [ ] Turkish language selection works
- [ ] All menus and dialogs display in Turkish
- [ ] Turkish characters display correctly
- [ ] Number formatting uses Turkish conventions
- [ ] Staking interface is properly translated
- [ ] Error messages appear in Turkish

#### Mobile Wallets
- [ ] Turkish language selection works
- [ ] All UI elements display in Turkish
- [ ] Turkish keyboard input works correctly
- [ ] Turkish characters display on all screen sizes
- [ ] Staking features are translated
- [ ] Push notifications appear in Turkish

#### Core Functionality
- [ ] RPC commands accept Turkish characters
- [ ] Wallet labels with Turkish characters work
- [ ] Transaction descriptions support Turkish
- [ ] Log files handle Turkish characters
- [ ] Database stores Turkish text correctly

## Troubleshooting

### Common Issues

#### Character Display Problems
- Ensure system has Turkish fonts installed
- Verify UTF-8 locale is set correctly
- Check terminal/console UTF-8 support

#### Input Problems  
- Verify Turkish keyboard layout is available
- Check input method configuration
- Ensure application has proper focus

#### Database Issues
- Verify UTF-8 encoding in database configuration
- Check file system UTF-8 support
- Ensure proper locale environment variables

### Environment Variables
Set these for proper Turkish support:
```bash
export LANG=tr_TR.UTF-8
export LC_ALL=tr_TR.UTF-8
export LC_CTYPE=tr_TR.UTF-8
```

## Development Guidelines

### Adding New Turkish Translations

#### Desktop Wallet
1. Edit `src/qt/locale/babachain_tr.ts`
2. Add new `<message>` entries with Turkish translations
3. Ensure proper UTF-8 encoding
4. Test with Qt Linguist tool

#### Android Wallet
1. Edit `BabaChain-AndroidMobileWallet/wallet/res/values-tr/strings.xml`
2. Add new `<string>` entries
3. Ensure proper XML escaping
4. Test on Android device

#### iOS Wallet
1. Edit `BabaChain-IOSMobileWallet/BabaChainWallet/tr.lproj/Localizable.strings`
2. Add new key-value pairs
3. Ensure proper UTF-8 encoding
4. Test on iOS device

### Code Guidelines
- Always use UTF-8 encoding for Turkish text
- Validate Turkish characters using utility functions
- Use Turkish locale for number/date formatting
- Test with actual Turkish text, not transliterated versions
- Consider Turkish collation rules for sorting

## Resources

### Turkish Language References
- [Turkish Language Association](https://www.tdk.gov.tr/)
- [Turkish Character Encoding Standards](https://en.wikipedia.org/wiki/Turkish_alphabet)
- [UTF-8 Turkish Character Codes](https://www.utf8-chartable.de/)

### Development Tools
- Qt Linguist for desktop translations
- Android Studio for mobile translations  
- Xcode for iOS translations
- Unicode text editors for UTF-8 validation

## Support

For Turkish language support issues:
1. Check this documentation first
2. Verify UTF-8 encoding is properly configured
3. Test with the provided validation scripts
4. Report issues with specific Turkish text examples
5. Include system locale and encoding information