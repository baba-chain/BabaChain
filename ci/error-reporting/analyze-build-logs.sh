#!/bin/bash
# BabaChain Build Log Analysis Script
# Captures and analyzes build logs for structured error reporting

set -euo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_DIR="${LOG_DIR:-${SCRIPT_DIR}/../../logs}"
PLATFORM="${PLATFORM:-unknown}"
BUILD_PHASE="${BUILD_PHASE:-unknown}"
OUTPUT_FORMAT="${OUTPUT_FORMAT:-github}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1" >&2
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1" >&2
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1" >&2
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1" >&2
}

# Create log directory
mkdir -p "$LOG_DIR"

# Function to capture command output and analyze errors
capture_and_analyze() {
    local command="$1"
    local phase="$2"
    local log_file="${LOG_DIR}/${PLATFORM}-${phase}-$(date +%Y%m%d-%H%M%S).log"
    
    log_info "Executing: $command"
    log_info "Logging to: $log_file"
    
    # Execute command and capture output
    local exit_code=0
    if ! eval "$command" 2>&1 | tee "$log_file"; then
        exit_code=$?
        log_error "Command failed with exit code: $exit_code"
    fi
    
    # Analyze the log file
    log_info "Analyzing build log for errors..."
    
    if python3 "${SCRIPT_DIR}/build-error-analyzer.py" "$log_file" "$PLATFORM" "$phase" "$OUTPUT_FORMAT"; then
        log_success "Build log analysis completed successfully"
    else
        log_error "Build log analysis detected errors"
        exit_code=1
    fi
    
    # Archive log file with metadata
    local archive_name="${LOG_DIR}/${PLATFORM}-${phase}-$(date +%Y%m%d-%H%M%S)"
    
    # Create metadata file
    cat > "${archive_name}.meta" << EOF
{
    "platform": "$PLATFORM",
    "build_phase": "$phase",
    "timestamp": "$(date -Iseconds)",
    "command": "$command",
    "exit_code": $exit_code,
    "log_file": "$(basename "$log_file")",
    "git_commit": "$(git rev-parse HEAD 2>/dev/null || echo 'unknown')",
    "git_branch": "$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo 'unknown')"
}
EOF
    
    return $exit_code
}

# Function to analyze existing log file
analyze_existing_log() {
    local log_file="$1"
    local phase="${2:-unknown}"
    
    if [[ ! -f "$log_file" ]]; then
        log_error "Log file not found: $log_file"
        return 1
    fi
    
    log_info "Analyzing existing log file: $log_file"
    
    if python3 "${SCRIPT_DIR}/build-error-analyzer.py" "$log_file" "$PLATFORM" "$phase" "$OUTPUT_FORMAT"; then
        log_success "Log analysis completed successfully"
        return 0
    else
        log_error "Log analysis detected errors"
        return 1
    fi
}

# Function to generate build environment diagnostics
generate_diagnostics() {
    local diag_file="${LOG_DIR}/${PLATFORM}-diagnostics-$(date +%Y%m%d-%H%M%S).log"
    
    log_info "Generating build environment diagnostics..."
    
    {
        echo "=== BabaChain Build Environment Diagnostics ==="
        echo "Timestamp: $(date -Iseconds)"
        echo "Platform: $PLATFORM"
        echo "Hostname: $(hostname)"
        echo "User: $(whoami)"
        echo "Working Directory: $(pwd)"
        echo ""
        
        echo "=== System Information ==="
        uname -a || echo "uname not available"
        echo ""
        
        if command -v lsb_release >/dev/null 2>&1; then
            echo "=== Linux Distribution ==="
            lsb_release -a
            echo ""
        fi
        
        if [[ "$OSTYPE" == "darwin"* ]]; then
            echo "=== macOS Information ==="
            sw_vers
            echo "Architecture: $(uname -m)"
            echo "Xcode Version: $(xcodebuild -version 2>/dev/null || echo 'Not available')"
            echo "SDK Path: $(xcrun --show-sdk-path 2>/dev/null || echo 'Not available')"
            echo ""
        fi
        
        echo "=== Environment Variables ==="
        env | grep -E "(CC|CXX|AR|STRIP|RANLIB|PATH|PKG_CONFIG|CFLAGS|CXXFLAGS|LDFLAGS)" | sort
        echo ""
        
        echo "=== Disk Space ==="
        df -h . || echo "df not available"
        echo ""
        
        echo "=== Memory Information ==="
        if command -v free >/dev/null 2>&1; then
            free -h
        elif [[ "$OSTYPE" == "darwin"* ]]; then
            vm_stat | head -10
        else
            echo "Memory info not available"
        fi
        echo ""
        
        echo "=== Build Tools ==="
        for tool in gcc g++ clang clang++ make cmake autoconf automake libtool pkg-config python3 git; do
            if command -v "$tool" >/dev/null 2>&1; then
                echo "$tool: $(command -v "$tool") - $("$tool" --version 2>/dev/null | head -1 || echo 'version unknown')"
            else
                echo "$tool: not found"
            fi
        done
        echo ""
        
        echo "=== Git Information ==="
        if command -v git >/dev/null 2>&1 && [[ -d .git ]]; then
            echo "Commit: $(git rev-parse HEAD)"
            echo "Branch: $(git rev-parse --abbrev-ref HEAD)"
            echo "Status:"
            git status --porcelain | head -10
        else
            echo "Git repository not available"
        fi
        echo ""
        
        echo "=== Dependencies Status ==="
        if [[ -d "depends" ]]; then
            echo "Depends directory exists"
            echo "Built dependencies:"
            ls -la depends/built/ 2>/dev/null | head -10 || echo "No built dependencies found"
            echo ""
            echo "Source cache:"
            ls -la depends/sources/ 2>/dev/null | head -10 || echo "No source cache found"
        else
            echo "Depends directory not found"
        fi
        echo ""
        
        echo "=== Configuration Files ==="
        for file in configure.ac Makefile.am CMakeLists.txt; do
            if [[ -f "$file" ]]; then
                echo "$file exists ($(wc -l < "$file") lines)"
            else
                echo "$file not found"
            fi
        done
        echo ""
        
        echo "=== Recent Build Artifacts ==="
        find . -name "*.o" -o -name "*.a" -o -name "*.so" -o -name "babachaind*" -o -name "babachain-cli*" 2>/dev/null | head -20 || echo "No build artifacts found"
        
    } > "$diag_file"
    
    log_success "Diagnostics saved to: $diag_file"
    
    # Also output to stdout if in GitHub Actions
    if [[ "${GITHUB_ACTIONS:-false}" == "true" ]]; then
        echo "::group::Build Environment Diagnostics"
        cat "$diag_file"
        echo "::endgroup::"
    fi
}

# Function to check for common build issues
check_common_issues() {
    log_info "Checking for common build issues..."
    
    local issues_found=0
    
    # Check disk space
    local available_space
    if command -v df >/dev/null 2>&1; then
        available_space=$(df . | tail -1 | awk '{print $4}')
        if [[ $available_space -lt 1048576 ]]; then  # Less than 1GB
            log_warn "Low disk space: $(df -h . | tail -1 | awk '{print $4}') available"
            ((issues_found++))
        fi
    fi
    
    # Check memory
    if command -v free >/dev/null 2>&1; then
        local available_mem
        available_mem=$(free -m | awk 'NR==2{print $7}')
        if [[ $available_mem -lt 512 ]]; then  # Less than 512MB
            log_warn "Low available memory: ${available_mem}MB"
            ((issues_found++))
        fi
    fi
    
    # Check required tools
    local required_tools=("gcc" "g++" "make" "autoconf" "automake" "libtool" "pkg-config" "python3")
    for tool in "${required_tools[@]}"; do
        if ! command -v "$tool" >/dev/null 2>&1; then
            log_warn "Required tool missing: $tool"
            ((issues_found++))
        fi
    done
    
    # Check for configure script
    if [[ ! -f "configure" ]] && [[ -f "configure.ac" ]]; then
        log_warn "configure script not found, but configure.ac exists. Run ./autogen.sh first."
        ((issues_found++))
    fi
    
    # Check for depends directory
    if [[ ! -d "depends" ]]; then
        log_warn "depends directory not found. This may cause dependency issues."
        ((issues_found++))
    fi
    
    if [[ $issues_found -eq 0 ]]; then
        log_success "No common build issues detected"
    else
        log_warn "Found $issues_found potential build issue(s)"
    fi
    
    return $issues_found
}

# Function to suggest fixes based on platform and errors
suggest_fixes() {
    local platform="$1"
    
    log_info "Generating platform-specific fix suggestions for $platform..."
    
    case "$platform" in
        linux-*)
            echo "::notice title=Linux Build Tips::Install build dependencies: sudo apt-get install build-essential libtool autotools-dev automake pkg-config"
            echo "::notice title=Linux Build Tips::For ARM64 cross-compilation: sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu"
            ;;
        macos-*)
            echo "::notice title=macOS Build Tips::Install Xcode command line tools: xcode-select --install"
            echo "::notice title=macOS Build Tips::Install Homebrew dependencies: brew install automake libtool pkg-config"
            echo "::notice title=macOS Build Tips::Check SDK path: xcrun --show-sdk-path"
            ;;
        windows-*)
            echo "::notice title=Windows Build Tips::Use MSYS2 environment for building"
            echo "::notice title=Windows Build Tips::Install MinGW toolchain: pacman -S mingw-w64-x86_64-toolchain"
            echo "::notice title=Windows Build Tips::Set proper PATH in MSYS2 environment"
            ;;
        *)
            echo "::notice title=General Build Tips::Check build dependencies and environment setup"
            ;;
    esac
}

# Main function
main() {
    local action="${1:-help}"
    
    case "$action" in
        "capture")
            local command="${2:-}"
            local phase="${3:-build}"
            
            if [[ -z "$command" ]]; then
                log_error "Usage: $0 capture <command> [phase]"
                exit 1
            fi
            
            generate_diagnostics
            check_common_issues || true  # Don't fail on warnings
            capture_and_analyze "$command" "$phase"
            ;;
            
        "analyze")
            local log_file="${2:-}"
            local phase="${3:-unknown}"
            
            if [[ -z "$log_file" ]]; then
                log_error "Usage: $0 analyze <log_file> [phase]"
                exit 1
            fi
            
            analyze_existing_log "$log_file" "$phase"
            ;;
            
        "diagnostics")
            generate_diagnostics
            check_common_issues
            suggest_fixes "$PLATFORM"
            ;;
            
        "help"|*)
            cat << EOF
BabaChain Build Log Analysis Script

Usage: $0 <action> [options]

Actions:
  capture <command> [phase]  - Execute command, capture output, and analyze
  analyze <log_file> [phase] - Analyze existing log file
  diagnostics               - Generate build environment diagnostics
  help                     - Show this help message

Environment Variables:
  PLATFORM      - Target platform (e.g., linux-x64, macos-arm64)
  BUILD_PHASE   - Build phase (e.g., configure, build, test)
  OUTPUT_FORMAT - Output format (github, json, markdown)
  LOG_DIR       - Directory for log files

Examples:
  $0 capture "make -j4" build
  $0 analyze build.log configure
  $0 diagnostics
EOF
            ;;
    esac
}

# Run main function with all arguments
main "$@"