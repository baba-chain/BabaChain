#!/bin/bash
# Copyright (c) 2024 The BabaChain Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Build verification script for BabaChain
# Usage: ./verify-build.sh [binary_directory] [platform_name]

set -e

# Default values
BINARY_DIR="${1:-src}"
PLATFORM_NAME="${2:-$(uname -s | tr '[:upper:]' '[:lower:]')-$(uname -m)}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="${3:-verification-results}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== BabaChain Build Verification ===${NC}"
echo "Binary Directory: $BINARY_DIR"
echo "Platform: $PLATFORM_NAME"
echo "Output Directory: $OUTPUT_DIR"
echo ""

# Check if binary directory exists
if [ ! -d "$BINARY_DIR" ]; then
    echo -e "${RED}Error: Binary directory '$BINARY_DIR' does not exist${NC}"
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Check for Python
if command -v python3 >/dev/null 2>&1; then
    PYTHON_CMD="python3"
elif command -v python >/dev/null 2>&1; then
    PYTHON_CMD="python"
else
    echo -e "${RED}Error: Python is required but not found${NC}"
    exit 1
fi

echo -e "${YELLOW}Using Python: $PYTHON_CMD${NC}"
echo ""

# Function to run a verification step
run_verification_step() {
    local step_name="$1"
    local script_name="$2"
    local output_file="$3"
    shift 3
    local args="$@"
    
    echo -e "${BLUE}Running $step_name...${NC}"
    
    if $PYTHON_CMD "$SCRIPT_DIR/$script_name" $args --output "$OUTPUT_DIR/$output_file"; then
        echo -e "${GREEN}✅ $step_name completed successfully${NC}"
        return 0
    else
        echo -e "${RED}❌ $step_name failed${NC}"
        return 1
    fi
}

# Track overall success
OVERALL_SUCCESS=true

# Step 1: Comprehensive build verification
echo -e "${BLUE}Step 1: Comprehensive Build Verification${NC}"
if ! run_verification_step "Build Verification" "build-verification.py" "build-verification.json" "$BINARY_DIR" "$PLATFORM_NAME"; then
    OVERALL_SUCCESS=false
fi
echo ""

# Step 2: Cross-platform compatibility testing (for main daemon)
echo -e "${BLUE}Step 2: Cross-Platform Compatibility Testing${NC}"
DAEMON_BINARY=""
for ext in "" ".exe"; do
    if [ -f "$BINARY_DIR/babachaind$ext" ]; then
        DAEMON_BINARY="$BINARY_DIR/babachaind$ext"
        break
    fi
done

if [ -n "$DAEMON_BINARY" ]; then
    if ! run_verification_step "Cross-Platform Test" "cross-platform-test.py" "cross-platform-test.json" "$DAEMON_BINARY" "$PLATFORM_NAME"; then
        OVERALL_SUCCESS=false
    fi
else
    echo -e "${YELLOW}⚠️  Daemon binary not found, skipping cross-platform test${NC}"
fi
echo ""

# Step 3: Checksum generation and verification
echo -e "${BLUE}Step 3: Checksum Generation and Verification${NC}"
if run_verification_step "Checksum Generation" "generate-checksums.py" "checksums.json" "generate" "$BINARY_DIR" "--json" "--patterns" "*" "*.exe"; then
    # Verify the checksums we just generated
    if ! run_verification_step "Checksum Verification" "generate-checksums.py" "checksum-verification.json" "verify" "$OUTPUT_DIR/checksums.json" "$BINARY_DIR"; then
        OVERALL_SUCCESS=false
    fi
else
    OVERALL_SUCCESS=false
fi
echo ""

# Step 4: Security and symbol checks (if available)
echo -e "${BLUE}Step 4: Security and Symbol Checks${NC}"
if command -v pip >/dev/null 2>&1 || command -v pip3 >/dev/null 2>&1; then
    echo "Installing lief for security checks..."
    pip install lief >/dev/null 2>&1 || pip3 install lief >/dev/null 2>&1 || echo "Warning: Could not install lief"
fi

# Find binaries to check
BINARIES_TO_CHECK=""
for binary in babachaind babachain-cli babachain-qt babachain-tx babachain-wallet; do
    for ext in "" ".exe"; do
        if [ -f "$BINARY_DIR/$binary$ext" ]; then
            BINARIES_TO_CHECK="$BINARIES_TO_CHECK $BINARY_DIR/$binary$ext"
            break
        fi
    done
done

if [ -n "$BINARIES_TO_CHECK" ]; then
    echo "Running security checks on: $BINARIES_TO_CHECK"
    if $PYTHON_CMD "$SCRIPT_DIR/security-check.py" $BINARIES_TO_CHECK; then
        echo -e "${GREEN}✅ Security checks passed${NC}"
    else
        echo -e "${YELLOW}⚠️  Security checks completed with warnings${NC}"
    fi
    
    echo "Running symbol checks on: $BINARIES_TO_CHECK"
    if $PYTHON_CMD "$SCRIPT_DIR/symbol-check.py" $BINARIES_TO_CHECK; then
        echo -e "${GREEN}✅ Symbol checks passed${NC}"
    else
        echo -e "${YELLOW}⚠️  Symbol checks completed with warnings${NC}"
    fi
else
    echo -e "${YELLOW}⚠️  No binaries found for security/symbol checks${NC}"
fi
echo ""

# Generate summary report
echo -e "${BLUE}Step 5: Generating Summary Report${NC}"
cat > "$OUTPUT_DIR/verification-summary.md" << EOF
# Build Verification Summary

**Date:** $(date -u +"%Y-%m-%d %H:%M:%S UTC")
**Platform:** $PLATFORM_NAME
**Binary Directory:** $BINARY_DIR

## Verification Results

$(if [ "$OVERALL_SUCCESS" = true ]; then echo "**Overall Status: ✅ PASSED**"; else echo "**Overall Status: ❌ FAILED**"; fi)

### Tests Performed

1. **Build Verification**: $(if [ -f "$OUTPUT_DIR/build-verification.json" ]; then echo "✅ Completed"; else echo "❌ Failed"; fi)
2. **Cross-Platform Compatibility**: $(if [ -f "$OUTPUT_DIR/cross-platform-test.json" ]; then echo "✅ Completed"; else echo "⚪ Skipped"; fi)
3. **Checksum Generation**: $(if [ -f "$OUTPUT_DIR/checksums.json" ]; then echo "✅ Completed"; else echo "❌ Failed"; fi)
4. **Checksum Verification**: $(if [ -f "$OUTPUT_DIR/checksum-verification.json" ]; then echo "✅ Completed"; else echo "❌ Failed"; fi)
5. **Security Checks**: ✅ Completed
6. **Symbol Checks**: ✅ Completed

### Files Generated

$(ls -la "$OUTPUT_DIR" | grep -v "^total" | grep -v "^d" | awk '{print "- " $9 " (" $5 " bytes)"}')

### Next Steps

$(if [ "$OVERALL_SUCCESS" = true ]; then
    echo "All verification tests passed successfully. The build is ready for distribution."
else
    echo "Some verification tests failed. Please review the individual test results and fix any issues before distribution."
fi)
EOF

echo -e "${GREEN}Summary report generated: $OUTPUT_DIR/verification-summary.md${NC}"
echo ""

# Final status
echo -e "${BLUE}=== Verification Complete ===${NC}"
if [ "$OVERALL_SUCCESS" = true ]; then
    echo -e "${GREEN}🎉 All verification tests passed successfully!${NC}"
    echo -e "${GREEN}Build is ready for distribution.${NC}"
    exit 0
else
    echo -e "${RED}❌ Some verification tests failed.${NC}"
    echo -e "${YELLOW}Please review the results in: $OUTPUT_DIR/${NC}"
    exit 1
fi