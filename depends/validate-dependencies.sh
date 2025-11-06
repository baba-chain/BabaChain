#!/bin/bash
# BabaChain Dependency Validation Script
# This script provides comprehensive validation of the dependency system

set -e

BASEDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERIFY_CACHE_DIR="$BASEDIR/verify-cache"
VALIDATION_LOG="$VERIFY_CACHE_DIR/validation.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1" | tee -a "$VALIDATION_LOG"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1" | tee -a "$VALIDATION_LOG"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1" | tee -a "$VALIDATION_LOG"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1" | tee -a "$VALIDATION_LOG"
}

# Initialize validation environment
init_validation() {
    mkdir -p "$VERIFY_CACHE_DIR"
    echo "$(date -u +"%Y-%m-%dT%H:%M:%SZ") - Validation started" > "$VALIDATION_LOG"
    log_info "Initializing dependency validation..."
    log_info "Base directory: $BASEDIR"
    log_info "Verification cache: $VERIFY_CACHE_DIR"
}

# Validate package definitions
validate_packages() {
    log_info "Validating package definitions..."
    
    local validation_errors=0
    local validation_warnings=0
    local packages_checked=0
    
    # Check if packages directory exists
    if [ ! -d "$BASEDIR/packages" ]; then
        log_error "Packages directory not found: $BASEDIR/packages"
        return 1
    fi
    
    # Validate each package file
    for package_file in "$BASEDIR/packages"/*.mk; do
        if [ "$package_file" = "$BASEDIR/packages/packages.mk" ]; then
            continue
        fi
        
        if [ ! -f "$package_file" ]; then
            continue
        fi
        
        local package_name=$(basename "$package_file" .mk)
        log_info "Validating package: $package_name"
        packages_checked=$((packages_checked + 1))
        
        # Check for required fields
        local has_version=$(grep -c "${package_name}_version" "$package_file" || echo "0")
        local has_url=$(grep -c "${package_name}_url" "$package_file" || echo "0")
        local has_sha256=$(grep -c "${package_name}_sha256" "$package_file" || echo "0")
        
        if [ "$has_version" -eq 0 ]; then
            log_error "Package $package_name missing version definition"
            validation_errors=$((validation_errors + 1))
        fi
        
        if [ "$has_url" -eq 0 ]; then
            log_warning "Package $package_name missing URL definition"
            validation_warnings=$((validation_warnings + 1))
        fi
        
        if [ "$has_sha256" -eq 0 ]; then
            log_warning "Package $package_name missing SHA256 checksum"
            validation_warnings=$((validation_warnings + 1))
        fi
        
        # Check for syntax errors
        if ! make -n -f "$package_file" >/dev/null 2>&1; then
            log_error "Package $package_name has syntax errors"
            validation_errors=$((validation_errors + 1))
        fi
    done
    
    # Generate package validation report
    cat > "$VERIFY_CACHE_DIR/package-validation-report.json" << EOF
{
    "timestamp": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
    "packages_checked": $packages_checked,
    "validation_errors": $validation_errors,
    "validation_warnings": $validation_warnings,
    "status": "$([ $validation_errors -eq 0 ] && echo "PASSED" || echo "FAILED")"
}
EOF
    
    log_info "Package validation completed: $packages_checked packages checked"
    log_info "Errors: $validation_errors, Warnings: $validation_warnings"
    
    if [ $validation_errors -eq 0 ]; then
        log_success "All package definitions are valid"
        return 0
    else
        log_error "Package validation failed with $validation_errors errors"
        return 1
    fi
}

# Validate build environment
validate_environment() {
    log_info "Validating build environment..."
    
    local env_errors=0
    local env_warnings=0
    
    # Check required tools
    local required_tools=("make" "gcc" "g++" "ar" "ranlib" "strip" "pkg-config")
    
    for tool in "${required_tools[@]}"; do
        if command -v "$tool" >/dev/null 2>&1; then
            log_success "Found required tool: $tool ($(command -v "$tool"))"
        else
            log_error "Missing required tool: $tool"
            env_errors=$((env_errors + 1))
        fi
    done
    
    # Check optional tools
    local optional_tools=("ccache" "curl" "wget" "python3" "autoconf" "automake" "libtool")
    
    for tool in "${optional_tools[@]}"; do
        if command -v "$tool" >/dev/null 2>&1; then
            log_success "Found optional tool: $tool ($(command -v "$tool"))"
        else
            log_warning "Missing optional tool: $tool"
            env_warnings=$((env_warnings + 1))
        fi
    done
    
    # Check disk space
    local available_space=$(df "$BASEDIR" | awk 'NR==2 {print $4}')
    local required_space=5242880  # 5GB in KB
    
    if [ "$available_space" -gt "$required_space" ]; then
        log_success "Sufficient disk space available: $(echo "$available_space" | awk '{print $1/1024/1024 " GB"}')"
    else
        log_warning "Low disk space: $(echo "$available_space" | awk '{print $1/1024/1024 " GB"}') available"
        env_warnings=$((env_warnings + 1))
    fi
    
    # Check memory
    if command -v free >/dev/null 2>&1; then
        local available_memory=$(free -m | awk 'NR==2{print $2}')
        if [ "$available_memory" -gt 2048 ]; then
            log_success "Sufficient memory available: ${available_memory}MB"
        else
            log_warning "Low memory: ${available_memory}MB available"
            env_warnings=$((env_warnings + 1))
        fi
    elif command -v vm_stat >/dev/null 2>&1; then
        log_info "macOS system detected - memory check skipped"
    else
        log_warning "Cannot determine available memory"
        env_warnings=$((env_warnings + 1))
    fi
    
    # Generate environment validation report
    cat > "$VERIFY_CACHE_DIR/environment-validation-report.json" << EOF
{
    "timestamp": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
    "environment_errors": $env_errors,
    "environment_warnings": $env_warnings,
    "available_space_kb": $available_space,
    "status": "$([ $env_errors -eq 0 ] && echo "PASSED" || echo "FAILED")"
}
EOF
    
    log_info "Environment validation completed"
    log_info "Errors: $env_errors, Warnings: $env_warnings"
    
    if [ $env_errors -eq 0 ]; then
        log_success "Build environment is valid"
        return 0
    else
        log_error "Environment validation failed with $env_errors errors"
        return 1
    fi
}

# Check dependency consistency
validate_consistency() {
    log_info "Checking dependency consistency..."
    
    local consistency_errors=0
    local consistency_warnings=0
    
    # Check for circular dependencies
    log_info "Checking for circular dependencies..."
    
    # This is a simplified check - a full dependency graph analysis would be more complex
    local packages_mk="$BASEDIR/packages/packages.mk"
    if [ -f "$packages_mk" ]; then
        # Check if packages.mk is properly formatted
        if grep -q "^packages" "$packages_mk"; then
            log_success "packages.mk appears to be properly formatted"
        else
            log_error "packages.mk may be malformed"
            consistency_errors=$((consistency_errors + 1))
        fi
    else
        log_error "packages.mk not found"
        consistency_errors=$((consistency_errors + 1))
    fi
    
    # Check host configurations
    log_info "Checking host configurations..."
    
    local hosts_dir="$BASEDIR/hosts"
    if [ -d "$hosts_dir" ]; then
        local host_files=("linux.mk" "darwin.mk" "mingw32.mk" "default.mk")
        for host_file in "${host_files[@]}"; do
            if [ -f "$hosts_dir/$host_file" ]; then
                log_success "Found host configuration: $host_file"
            else
                log_warning "Missing host configuration: $host_file"
                consistency_warnings=$((consistency_warnings + 1))
            fi
        done
    else
        log_error "Hosts directory not found: $hosts_dir"
        consistency_errors=$((consistency_errors + 1))
    fi
    
    # Check builder configurations
    log_info "Checking builder configurations..."
    
    local builders_dir="$BASEDIR/builders"
    if [ -d "$builders_dir" ]; then
        local builder_files=("linux.mk" "darwin.mk" "default.mk")
        for builder_file in "${builder_files[@]}"; do
            if [ -f "$builders_dir/$builder_file" ]; then
                log_success "Found builder configuration: $builder_file"
            else
                log_warning "Missing builder configuration: $builder_file"
                consistency_warnings=$((consistency_warnings + 1))
            fi
        done
    else
        log_error "Builders directory not found: $builders_dir"
        consistency_errors=$((consistency_errors + 1))
    fi
    
    # Generate consistency validation report
    cat > "$VERIFY_CACHE_DIR/consistency-validation-report.json" << EOF
{
    "timestamp": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
    "consistency_errors": $consistency_errors,
    "consistency_warnings": $consistency_warnings,
    "status": "$([ $consistency_errors -eq 0 ] && echo "PASSED" || echo "FAILED")"
}
EOF
    
    log_info "Consistency validation completed"
    log_info "Errors: $consistency_errors, Warnings: $consistency_warnings"
    
    if [ $consistency_errors -eq 0 ]; then
        log_success "Dependency consistency is valid"
        return 0
    else
        log_error "Consistency validation failed with $consistency_errors errors"
        return 1
    fi
}

# Generate comprehensive validation status
generate_status() {
    log_info "Generating validation status report..."
    
    local package_status="UNKNOWN"
    local environment_status="UNKNOWN"
    local consistency_status="UNKNOWN"
    
    # Read individual report statuses
    if [ -f "$VERIFY_CACHE_DIR/package-validation-report.json" ]; then
        package_status=$(grep '"status"' "$VERIFY_CACHE_DIR/package-validation-report.json" | cut -d'"' -f4)
    fi
    
    if [ -f "$VERIFY_CACHE_DIR/environment-validation-report.json" ]; then
        environment_status=$(grep '"status"' "$VERIFY_CACHE_DIR/environment-validation-report.json" | cut -d'"' -f4)
    fi
    
    if [ -f "$VERIFY_CACHE_DIR/consistency-validation-report.json" ]; then
        consistency_status=$(grep '"status"' "$VERIFY_CACHE_DIR/consistency-validation-report.json" | cut -d'"' -f4)
    fi
    
    # Determine overall status
    local overall_status="PASSED"
    if [ "$package_status" = "FAILED" ] || [ "$environment_status" = "FAILED" ] || [ "$consistency_status" = "FAILED" ]; then
        overall_status="FAILED"
    fi
    
    # Generate comprehensive status report
    cat > "$VERIFY_CACHE_DIR/validation-status.json" << EOF
{
    "timestamp": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
    "overall_status": "$overall_status",
    "package_validation": "$package_status",
    "environment_validation": "$environment_status",
    "consistency_validation": "$consistency_status",
    "validation_log": "$VALIDATION_LOG"
}
EOF
    
    # Generate human-readable status
    cat > "$VERIFY_CACHE_DIR/validation-status.md" << EOF
# Dependency Validation Status Report

**Generated:** $(date -u +"%Y-%m-%d %H:%M:%S UTC")
**Overall Status:** $overall_status

## Validation Results

| Component | Status |
|-----------|--------|
| Package Definitions | $package_status |
| Build Environment | $environment_status |
| Dependency Consistency | $consistency_status |

## Summary

$(if [ "$overall_status" = "PASSED" ]; then
    echo "✅ All dependency validations passed successfully."
else
    echo "❌ One or more dependency validations failed. Check individual reports for details."
fi)

## Next Steps

$(if [ "$overall_status" = "PASSED" ]; then
    echo "- Dependencies are ready for building"
    echo "- Consider running \`make check-packages\` to verify cached dependencies"
else
    echo "- Review validation errors in the log file: $VALIDATION_LOG"
    echo "- Fix any missing tools or configuration issues"
    echo "- Re-run validation after making corrections"
fi)

## Log File

Full validation log: \`$VALIDATION_LOG\`
EOF
    
    log_info "Validation status report generated"
    log_info "Overall status: $overall_status"
    
    # Display summary
    echo
    echo "=== VALIDATION SUMMARY ==="
    echo "Package Definitions: $package_status"
    echo "Build Environment: $environment_status"
    echo "Dependency Consistency: $consistency_status"
    echo "Overall Status: $overall_status"
    echo
    
    if [ "$overall_status" = "PASSED" ]; then
        log_success "All dependency validations passed!"
        return 0
    else
        log_error "Dependency validation failed. Check reports in $VERIFY_CACHE_DIR"
        return 1
    fi
}

# Clean validation cache
clean_validation() {
    log_info "Cleaning validation cache..."
    
    if [ -d "$VERIFY_CACHE_DIR" ]; then
        rm -rf "$VERIFY_CACHE_DIR"
        log_success "Validation cache cleaned"
    else
        log_info "No validation cache to clean"
    fi
}

# Main function
main() {
    local action="${1:-validate}"
    
    case "$action" in
        "validate")
            init_validation
            validate_packages
            validate_environment
            validate_consistency
            generate_status
            ;;
        "packages")
            init_validation
            validate_packages
            ;;
        "environment")
            init_validation
            validate_environment
            ;;
        "consistency")
            init_validation
            validate_consistency
            ;;
        "status")
            generate_status
            ;;
        "clean")
            clean_validation
            ;;
        *)
            echo "Usage: $0 {validate|packages|environment|consistency|status|clean}"
            echo
            echo "Commands:"
            echo "  validate     - Run all validations (default)"
            echo "  packages     - Validate package definitions only"
            echo "  environment  - Validate build environment only"
            echo "  consistency  - Check dependency consistency only"
            echo "  status       - Generate status report from existing validations"
            echo "  clean        - Clean validation cache"
            exit 1
            ;;
    esac
}

# Run main function with all arguments
main "$@"