# BabaChain Build Verification System

This directory contains comprehensive build verification tools for BabaChain binaries. These tools ensure that built binaries are functional, secure, and compatible across different platforms.

## Overview

The build verification system consists of several components:

1. **Build Verification** (`build-verification.py`) - Tests binary functionality and generates checksums
2. **Cross-Platform Testing** (`cross-platform-test.py`) - Verifies platform-specific compatibility
3. **Checksum Management** (`generate-checksums.py`) - Generates and verifies file checksums
4. **Security Checks** (`security-check.py`) - Existing security feature verification
5. **Symbol Checks** (`symbol-check.py`) - Existing symbol and library dependency checks

## Quick Start

### Using the Wrapper Script

The easiest way to run all verification tests is using the wrapper script:

```bash
# Verify binaries in the src directory for current platform
./contrib/devtools/verify-build.sh

# Verify binaries in a specific directory for a specific platform
./contrib/devtools/verify-build.sh release/babachain-linux-x64 linux-x64

# Specify custom output directory
./contrib/devtools/verify-build.sh src linux-x64 my-verification-results
```

### Using Make Targets

You can also use the integrated Make targets:

```bash
# Run comprehensive build verification
make check-build-verification

# Generate checksums only
make check-checksums

# Run security and symbol checks
make check-security check-symbols
```

## Individual Tools

### Build Verification Tool

Tests binary functionality and generates comprehensive reports:

```bash
python3 contrib/devtools/build-verification.py src linux-x64 --output verification.json
```

Features:
- Tests binary execution and version commands
- Verifies architecture compatibility
- Generates multiple hash types (SHA256, SHA1, MD5)
- Tests daemon-specific functionality
- Creates detailed JSON reports

### Cross-Platform Testing Tool

Verifies platform-specific compatibility:

```bash
python3 contrib/devtools/cross-platform-test.py src/babachaind linux-x64 --output compat.json
```

Features:
- Tests library dependencies
- Verifies architecture matching
- Checks platform-specific requirements (GLIBC, macOS version, etc.)
- Tests security features
- Validates code signing (macOS)

### Checksum Management Tool

Generates and verifies file checksums:

```bash
# Generate checksums
python3 contrib/devtools/generate-checksums.py generate src --output checksums.json --json

# Generate traditional SHA256SUMS file
python3 contrib/devtools/generate-checksums.py generate src --output SHA256SUMS

# Verify checksums
python3 contrib/devtools/generate-checksums.py verify checksums.json src
```

Features:
- Multiple hash algorithms (SHA256, SHA1, MD5)
- JSON and traditional text formats
- Batch verification
- File size tracking

## CI/CD Integration

The build verification system is integrated into the GitHub Actions workflow:

1. **Basic Functionality Tests** - Version and help command verification
2. **Comprehensive Verification** - Full build verification suite
3. **Cross-Platform Testing** - Platform-specific compatibility checks
4. **Checksum Generation** - Automatic checksum creation and verification
5. **Security Checks** - Binary security feature validation

Results are uploaded as artifacts for each platform build.

## Output Files

The verification system generates several output files:

- `build-verification.json` - Comprehensive build test results
- `cross-platform-test.json` - Platform compatibility test results
- `checksums.json` - Detailed checksum information
- `SHA256SUMS` - Traditional checksum file
- `checksum-verification.json` - Checksum verification results
- `verification-summary.md` - Human-readable summary report

## Platform Support

The verification system supports all BabaChain target platforms:

- **Linux**: x86_64, ARM64 (aarch64)
- **macOS**: x86_64 (Intel), ARM64 (Apple Silicon)
- **Windows**: x86_64

Platform-specific tests include:
- Linux: GLIBC version, security features, architecture validation
- macOS: Version compatibility, code signing, universal binary support
- Windows: PE format validation, DLL dependencies, architecture matching

## Requirements

- Python 3.6+
- `lief` package for security checks (automatically installed in CI)
- Platform-specific tools:
  - Linux: `ldd`, `readelf`, `file`
  - macOS: `otool`, `lipo`, `codesign`, `file`
  - Windows: `objdump`, `file`

## Error Handling

The verification system provides detailed error reporting:

- Individual test failures are logged with specific error messages
- Overall success/failure status is clearly indicated
- Warnings are distinguished from critical failures
- JSON output includes full error context for debugging

## Security Considerations

The verification system includes several security-focused checks:

- Binary security features (PIE, RELRO, stack canaries, etc.)
- Symbol table validation
- Library dependency verification
- Code signing validation (macOS)
- Architecture validation to prevent binary substitution

## Extending the System

To add new verification tests:

1. Add test methods to the appropriate class in the verification scripts
2. Update the `run_verification()` or `run_all_tests()` methods
3. Add corresponding output fields to the results dictionary
4. Update the CI workflow if needed
5. Document the new tests in this README

## Troubleshooting

Common issues and solutions:

- **Python not found**: Ensure Python 3.6+ is installed and in PATH
- **lief import error**: Install with `pip install lief`
- **Platform tools missing**: Install development tools for your platform
- **Permission denied**: Ensure binaries have execute permissions
- **Architecture mismatch**: Verify you're testing the correct binary for your platform

For more detailed troubleshooting, check the JSON output files which contain specific error messages and context.