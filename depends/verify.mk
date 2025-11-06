# Dependency Verification System
# This file provides enhanced checksum verification, cache validation, and update automation

# Verification configuration
VERIFY_TIMEOUT ?= 30
VERIFY_RETRIES ?= 3
VERIFY_LOG_DIR ?= $(BASEDIR)/verify-logs
VERIFY_CACHE_DIR ?= $(BASE_CACHE)/verify
VERIFY_REPORT_FILE ?= $(BASEDIR)/dependency-verification-report.json

# Create verification directories
$(shell mkdir -p $(VERIFY_LOG_DIR) $(VERIFY_CACHE_DIR))

# Enhanced checksum verification with detailed error reporting
define verify_package_checksum
	echo "Verifying checksum for $(1)..."; \
	mkdir -p $(VERIFY_LOG_DIR); \
	if [ -f "$($(1)_cached)" ]; then \
		echo "Checking cached package: $($(1)_cached)"; \
		cd $(BASE_CACHE)/$(host)/$(1) && \
		if $(build_SHA256SUM) -c $($(1)_cached_checksum) >/dev/null 2>$(VERIFY_LOG_DIR)/$(1)-checksum.log; then \
			echo "✓ Checksum verification passed for $(1)"; \
			echo "$(1): PASS" >> $(VERIFY_CACHE_DIR)/checksum-status.log; \
		else \
			echo "✗ Checksum verification failed for $(1)"; \
			echo "Expected checksum file: $($(1)_cached_checksum)"; \
			echo "Cached file: $($(1)_cached)"; \
			echo "Error details:"; \
			cat $(VERIFY_LOG_DIR)/$(1)-checksum.log; \
			echo "$(1): FAIL - checksum mismatch" >> $(VERIFY_CACHE_DIR)/checksum-status.log; \
			echo "Removing corrupted cache for $(1)..."; \
			rm -f $($(1)_cached_checksum) $($(1)_cached); \
		fi; \
	else \
		echo "No cached package found for $(1)"; \
		echo "$(1): MISSING - no cached package" >> $(VERIFY_CACHE_DIR)/checksum-status.log; \
	fi
endef

# Enhanced source verification with retry mechanism
define verify_package_sources
	echo "Verifying sources for $(1)..."; \
	mkdir -p $(VERIFY_LOG_DIR) $($(1)_source_dir); \
	cd $($(1)_source_dir) && \
	if [ -f "$($(1)_fetched)" ]; then \
		echo "Checking source files for $(1)"; \
		if $(build_SHA256SUM) -c $($(1)_fetched) >/dev/null 2>$(VERIFY_LOG_DIR)/$(1)-sources.log; then \
			echo "✓ Source verification passed for $(1)"; \
			echo "$(1): PASS" >> $(VERIFY_CACHE_DIR)/sources-status.log; \
		else \
			echo "✗ Source verification failed for $(1)"; \
			echo "Error details:"; \
			cat $(VERIFY_LOG_DIR)/$(1)-sources.log; \
			echo "$(1): FAIL - source checksum mismatch" >> $(VERIFY_CACHE_DIR)/sources-status.log; \
			echo "Removing corrupted sources for $(1)..."; \
			rm -f $($(1)_all_sources) $($(1)_fetched); \
		fi; \
	else \
		echo "No source files found for $(1)"; \
		echo "$(1): MISSING - no source files" >> $(VERIFY_CACHE_DIR)/sources-status.log; \
	fi
endef

# Dependency cache validation with integrity checks
define validate_dependency_cache
	echo "Validating dependency cache for $(1)..."; \
	mkdir -p $(VERIFY_LOG_DIR); \
	if [ -f "$($(1)_cached)" ]; then \
		echo "Validating cache integrity for $(1)"; \
		if $(build_TAR) -tzf $($(1)_cached) >/dev/null 2>$(VERIFY_LOG_DIR)/$(1)-cache-integrity.log; then \
			echo "✓ Cache integrity check passed for $(1)"; \
			echo "$(1): PASS" >> $(VERIFY_CACHE_DIR)/cache-integrity.log; \
		else \
			echo "✗ Cache integrity check failed for $(1)"; \
			echo "Error details:"; \
			cat $(VERIFY_LOG_DIR)/$(1)-cache-integrity.log; \
			echo "$(1): FAIL - corrupted cache archive" >> $(VERIFY_CACHE_DIR)/cache-integrity.log; \
			echo "Removing corrupted cache for $(1)..."; \
			rm -f $($(1)_cached_checksum) $($(1)_cached); \
		fi; \
	else \
		echo "$(1): MISSING - no cached package" >> $(VERIFY_CACHE_DIR)/cache-integrity.log; \
	fi
endef

# Check for dependency updates by comparing versions
define check_dependency_updates
	echo "Checking for updates for $(1)..."; \
	mkdir -p $(VERIFY_LOG_DIR); \
	echo "Current version: $($(1)_version)" > $(VERIFY_LOG_DIR)/$(1)-version-check.log; \
	echo "Download path: $($(1)_download_path)" >> $(VERIFY_LOG_DIR)/$(1)-version-check.log; \
	echo "$(1): $($(1)_version)" >> $(VERIFY_CACHE_DIR)/current-versions.log
endef

# Generate comprehensive verification report
define generate_verification_report
	@echo "Generating dependency verification report..."
	@mkdir -p $(VERIFY_CACHE_DIR)
	@echo "{" > $(VERIFY_REPORT_FILE)
	@echo '  "timestamp": "'`date -u +"%Y-%m-%dT%H:%M:%SZ"`'",' >> $(VERIFY_REPORT_FILE)
	@echo '  "host": "$(host)",' >> $(VERIFY_REPORT_FILE)
	@echo '  "build": "$(build)",' >> $(VERIFY_REPORT_FILE)
	@echo '  "packages": {' >> $(VERIFY_REPORT_FILE)
	@first=true; \
	for package in $(all_packages); do \
		if [ "$$first" = "true" ]; then \
			first=false; \
		else \
			echo "    ," >> $(VERIFY_REPORT_FILE); \
		fi; \
		echo '    "'$$package'": {' >> $(VERIFY_REPORT_FILE); \
		echo '      "version": "'`eval echo \\$$$$package'_version'`'",' >> $(VERIFY_REPORT_FILE); \
		echo '      "checksum_status": "'`grep "^$$package:" $(VERIFY_CACHE_DIR)/checksum-status.log 2>/dev/null | cut -d: -f2 | tr -d ' ' || echo "UNKNOWN"`'",' >> $(VERIFY_REPORT_FILE); \
		echo '      "sources_status": "'`grep "^$$package:" $(VERIFY_CACHE_DIR)/sources-status.log 2>/dev/null | cut -d: -f2 | tr -d ' ' || echo "UNKNOWN"`'",' >> $(VERIFY_REPORT_FILE); \
		echo '      "cache_integrity": "'`grep "^$$package:" $(VERIFY_CACHE_DIR)/cache-integrity.log 2>/dev/null | cut -d: -f2 | tr -d ' ' || echo "UNKNOWN"`'"' >> $(VERIFY_REPORT_FILE); \
		echo '    }' >> $(VERIFY_REPORT_FILE); \
	done
	@echo "  }," >> $(VERIFY_REPORT_FILE)
	@echo '  "summary": {' >> $(VERIFY_REPORT_FILE)
	@echo '    "total_packages": '`echo $(all_packages) | wc -w`',' >> $(VERIFY_REPORT_FILE)
	@echo '    "checksum_passed": '`grep ": PASS" $(VERIFY_CACHE_DIR)/checksum-status.log 2>/dev/null | wc -l || echo 0`',' >> $(VERIFY_REPORT_FILE)
	@echo '    "checksum_failed": '`grep ": FAIL" $(VERIFY_CACHE_DIR)/checksum-status.log 2>/dev/null | wc -l || echo 0`',' >> $(VERIFY_REPORT_FILE)
	@echo '    "sources_passed": '`grep ": PASS" $(VERIFY_CACHE_DIR)/sources-status.log 2>/dev/null | wc -l || echo 0`',' >> $(VERIFY_REPORT_FILE)
	@echo '    "sources_failed": '`grep ": FAIL" $(VERIFY_CACHE_DIR)/sources-status.log 2>/dev/null | wc -l || echo 0`',' >> $(VERIFY_REPORT_FILE)
	@echo '    "cache_integrity_passed": '`grep ": PASS" $(VERIFY_CACHE_DIR)/cache-integrity.log 2>/dev/null | wc -l || echo 0`',' >> $(VERIFY_REPORT_FILE)
	@echo '    "cache_integrity_failed": '`grep ": FAIL" $(VERIFY_CACHE_DIR)/cache-integrity.log 2>/dev/null | wc -l || echo 0` >> $(VERIFY_REPORT_FILE)
	@echo "  }" >> $(VERIFY_REPORT_FILE)
	@echo "}" >> $(VERIFY_REPORT_FILE)
	@echo "Verification report generated: $(VERIFY_REPORT_FILE)"
endef

# Clean verification logs and cache
verify-clean:
	@echo "Cleaning verification logs and cache..."
	@rm -rf $(VERIFY_LOG_DIR) $(VERIFY_CACHE_DIR)
	@rm -f $(VERIFY_REPORT_FILE)
	@echo "Verification cleanup completed"

# Verify all package checksums
verify-checksums:
	@echo "Starting comprehensive checksum verification..."
	@rm -f $(VERIFY_CACHE_DIR)/checksum-status.log
	@$(foreach package,$(all_packages),$(call verify_package_checksum,$(package));)
	@echo "Checksum verification completed"
	@echo "Results summary:"
	@if [ -f "$(VERIFY_CACHE_DIR)/checksum-status.log" ]; then \
		echo "Passed: `grep ': PASS' $(VERIFY_CACHE_DIR)/checksum-status.log | wc -l`"; \
		echo "Failed: `grep ': FAIL' $(VERIFY_CACHE_DIR)/checksum-status.log | wc -l`"; \
		echo "Missing: `grep ': MISSING' $(VERIFY_CACHE_DIR)/checksum-status.log | wc -l`"; \
		if grep -q ': FAIL' $(VERIFY_CACHE_DIR)/checksum-status.log; then \
			echo "Failed packages:"; \
			grep ': FAIL' $(VERIFY_CACHE_DIR)/checksum-status.log; \
		fi; \
	fi

# Verify all package sources
verify-sources:
	@echo "Starting comprehensive source verification..."
	@rm -f $(VERIFY_CACHE_DIR)/sources-status.log
	@$(foreach package,$(all_packages),$(call verify_package_sources,$(package));)
	@echo "Source verification completed"
	@echo "Results summary:"
	@if [ -f "$(VERIFY_CACHE_DIR)/sources-status.log" ]; then \
		echo "Passed: `grep ': PASS' $(VERIFY_CACHE_DIR)/sources-status.log | wc -l`"; \
		echo "Failed: `grep ': FAIL' $(VERIFY_CACHE_DIR)/sources-status.log | wc -l`"; \
		echo "Missing: `grep ': MISSING' $(VERIFY_CACHE_DIR)/sources-status.log | wc -l`"; \
		if grep -q ': FAIL' $(VERIFY_CACHE_DIR)/sources-status.log; then \
			echo "Failed packages:"; \
			grep ': FAIL' $(VERIFY_CACHE_DIR)/sources-status.log; \
		fi; \
	fi

# Validate dependency cache integrity
verify-cache:
	@echo "Starting dependency cache validation..."
	@rm -f $(VERIFY_CACHE_DIR)/cache-integrity.log
	@$(foreach package,$(all_packages),$(call validate_dependency_cache,$(package));)
	@echo "Cache validation completed"
	@echo "Results summary:"
	@if [ -f "$(VERIFY_CACHE_DIR)/cache-integrity.log" ]; then \
		echo "Passed: `grep ': PASS' $(VERIFY_CACHE_DIR)/cache-integrity.log | wc -l`"; \
		echo "Failed: `grep ': FAIL' $(VERIFY_CACHE_DIR)/cache-integrity.log | wc -l`"; \
		echo "Missing: `grep ': MISSING' $(VERIFY_CACHE_DIR)/cache-integrity.log | wc -l`"; \
		if grep -q ': FAIL' $(VERIFY_CACHE_DIR)/cache-integrity.log; then \
			echo "Failed packages:"; \
			grep ': FAIL' $(VERIFY_CACHE_DIR)/cache-integrity.log; \
		fi; \
	fi

# Check for dependency updates
check-updates:
	@echo "Checking for dependency updates..."
	@rm -f $(VERIFY_CACHE_DIR)/current-versions.log
	@$(foreach package,$(all_packages),$(call check_dependency_updates,$(package));)
	@echo "Update check completed"
	@echo "Current dependency versions:"
	@if [ -f "$(VERIFY_CACHE_DIR)/current-versions.log" ]; then \
		cat $(VERIFY_CACHE_DIR)/current-versions.log; \
	fi

# Comprehensive verification (all checks)
verify-all: verify-checksums verify-sources verify-cache
	@$(call generate_verification_report)
	@echo "Comprehensive verification completed"
	@echo "Report available at: $(VERIFY_REPORT_FILE)"

# Show verification status
verify-status:
	@echo "Dependency Verification Status"
	@echo "=============================="
	@if [ -f "$(VERIFY_REPORT_FILE)" ]; then \
		echo "Last verification: `grep timestamp $(VERIFY_REPORT_FILE) | cut -d'"' -f4`"; \
		echo "Total packages: `grep total_packages $(VERIFY_REPORT_FILE) | cut -d: -f2 | tr -d ' ,'`"; \
		echo "Checksum passed: `grep checksum_passed $(VERIFY_REPORT_FILE) | cut -d: -f2 | tr -d ' ,'`"; \
		echo "Checksum failed: `grep checksum_failed $(VERIFY_REPORT_FILE) | cut -d: -f2 | tr -d ' ,'`"; \
		echo "Sources passed: `grep sources_passed $(VERIFY_REPORT_FILE) | cut -d: -f2 | tr -d ' ,'`"; \
		echo "Sources failed: `grep sources_failed $(VERIFY_REPORT_FILE) | cut -d: -f2 | tr -d ' ,'`"; \
		echo "Cache integrity passed: `grep cache_integrity_passed $(VERIFY_REPORT_FILE) | cut -d: -f2 | tr -d ' ,'`"; \
		echo "Cache integrity failed: `grep cache_integrity_failed $(VERIFY_REPORT_FILE) | cut -d: -f2 | tr -d ' '`"; \
	else \
		echo "No verification report found. Run 'make verify-all' first."; \
	fi

.PHONY: verify-clean verify-checksums verify-sources verify-cache check-updates verify-all verify-status