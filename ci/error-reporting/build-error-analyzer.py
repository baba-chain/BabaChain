#!/usr/bin/env python3
"""
BabaChain Build Error Analyzer
Analyzes build logs and provides structured error reporting for CI/CD
"""

import sys
import re
import json
import os
from datetime import datetime
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass, asdict
from enum import Enum

class ErrorCategory(Enum):
    DEPENDENCY = "dependency"
    COMPILATION = "compilation"
    LINKING = "linking"
    CONFIGURATION = "configuration"
    PLATFORM = "platform"
    NETWORK = "network"
    UNKNOWN = "unknown"

class ErrorSeverity(Enum):
    CRITICAL = "critical"
    HIGH = "high"
    MEDIUM = "medium"
    LOW = "low"

@dataclass
class BuildError:
    category: ErrorCategory
    severity: ErrorSeverity
    message: str
    context: str
    file_path: Optional[str] = None
    line_number: Optional[int] = None
    suggestion: Optional[str] = None
    documentation_link: Optional[str] = None

@dataclass
class BuildReport:
    platform: str
    timestamp: str
    build_phase: str
    success: bool
    errors: List[BuildError]
    warnings: List[BuildError]
    summary: str
    build_time: Optional[float] = None
    artifacts_generated: List[str] = None

class BuildErrorAnalyzer:
    def __init__(self):
        self.error_patterns = self._initialize_error_patterns()
        self.suggestion_map = self._initialize_suggestions()
        
    def _initialize_error_patterns(self) -> Dict[str, Tuple[ErrorCategory, ErrorSeverity, str]]:
        """Initialize regex patterns for common build errors"""
        return {
            # Dependency errors
            r"Package .* was not found": (ErrorCategory.DEPENDENCY, ErrorSeverity.CRITICAL, 
                "Missing package dependency"),
            r"No package '.*' found": (ErrorCategory.DEPENDENCY, ErrorSeverity.CRITICAL,
                "pkg-config package not found"),
            r"configure: error: .* not found": (ErrorCategory.DEPENDENCY, ErrorSeverity.CRITICAL,
                "Configure script dependency check failed"),
            r"fatal error: '.*\.h' file not found": (ErrorCategory.DEPENDENCY, ErrorSeverity.CRITICAL,
                "Missing header file"),
            r"cannot find -l.*": (ErrorCategory.LINKING, ErrorSeverity.CRITICAL,
                "Missing library for linking"),
            
            # Compilation errors
            r"error: '.*' was not declared": (ErrorCategory.COMPILATION, ErrorSeverity.HIGH,
                "Undeclared identifier"),
            r"error: no matching function": (ErrorCategory.COMPILATION, ErrorSeverity.HIGH,
                "Function signature mismatch"),
            r"error: invalid conversion": (ErrorCategory.COMPILATION, ErrorSeverity.HIGH,
                "Type conversion error"),
            r"error: expected '.*' before": (ErrorCategory.COMPILATION, ErrorSeverity.HIGH,
                "Syntax error"),
            
            # Linking errors
            r"undefined reference to": (ErrorCategory.LINKING, ErrorSeverity.CRITICAL,
                "Undefined symbol during linking"),
            r"multiple definition of": (ErrorCategory.LINKING, ErrorSeverity.HIGH,
                "Symbol defined multiple times"),
            r"relocation .* can not be used": (ErrorCategory.LINKING, ErrorSeverity.HIGH,
                "Relocation error in linking"),
            
            # Configuration errors
            r"configure: error:": (ErrorCategory.CONFIGURATION, ErrorSeverity.CRITICAL,
                "Configuration script error"),
            r"autoreconf: command not found": (ErrorCategory.CONFIGURATION, ErrorSeverity.CRITICAL,
                "Missing autotools"),
            r"aclocal: command not found": (ErrorCategory.CONFIGURATION, ErrorSeverity.CRITICAL,
                "Missing automake"),
            
            # Platform-specific errors
            r"unsupported target": (ErrorCategory.PLATFORM, ErrorSeverity.CRITICAL,
                "Unsupported target platform"),
            r"cross-compilation.*failed": (ErrorCategory.PLATFORM, ErrorSeverity.CRITICAL,
                "Cross-compilation failure"),
            r"SDK.*not found": (ErrorCategory.PLATFORM, ErrorSeverity.CRITICAL,
                "Missing platform SDK"),
            
            # Network errors
            r"Failed to download": (ErrorCategory.NETWORK, ErrorSeverity.MEDIUM,
                "Download failure"),
            r"Connection timed out": (ErrorCategory.NETWORK, ErrorSeverity.MEDIUM,
                "Network timeout"),
            r"Name or service not known": (ErrorCategory.NETWORK, ErrorSeverity.MEDIUM,
                "DNS resolution failure"),
        }
    
    def _initialize_suggestions(self) -> Dict[ErrorCategory, Dict[str, str]]:
        """Initialize suggestions for different error categories"""
        return {
            ErrorCategory.DEPENDENCY: {
                "missing_package": "Install the missing package using your system package manager",
                "pkg_config": "Ensure pkg-config is installed and PKG_CONFIG_PATH is set correctly",
                "header_file": "Install development headers for the missing library",
                "library": "Install the library or check library search paths"
            },
            ErrorCategory.COMPILATION: {
                "undeclared": "Check if the required header is included",
                "function_mismatch": "Verify function signature matches the declaration",
                "type_conversion": "Add explicit type casting or fix type mismatch",
                "syntax": "Check for missing semicolons, brackets, or other syntax issues"
            },
            ErrorCategory.LINKING: {
                "undefined_reference": "Ensure all required libraries are linked",
                "multiple_definition": "Check for duplicate symbol definitions",
                "relocation": "Use position-independent code flags if needed"
            },
            ErrorCategory.CONFIGURATION: {
                "configure_error": "Check configure.ac and ensure all dependencies are available",
                "missing_autotools": "Install autotools: autoconf, automake, libtool",
                "aclocal": "Install automake package"
            },
            ErrorCategory.PLATFORM: {
                "unsupported_target": "Verify the target platform is supported",
                "cross_compilation": "Check cross-compilation toolchain setup",
                "missing_sdk": "Install the required platform SDK"
            },
            ErrorCategory.NETWORK: {
                "download_failure": "Check network connectivity and retry",
                "timeout": "Increase timeout values or check network stability",
                "dns_failure": "Check DNS configuration and network connectivity"
            }
        }
    
    def analyze_log(self, log_content: str, platform: str, build_phase: str) -> BuildReport:
        """Analyze build log and generate structured error report"""
        errors = []
        warnings = []
        
        lines = log_content.split('\n')
        for i, line in enumerate(lines):
            # Check for errors
            if self._is_error_line(line):
                error = self._parse_error_line(line, i + 1)
                if error:
                    errors.append(error)
            
            # Check for warnings
            elif self._is_warning_line(line):
                warning = self._parse_warning_line(line, i + 1)
                if warning:
                    warnings.append(warning)
        
        # Generate summary
        success = len(errors) == 0
        summary = self._generate_summary(errors, warnings, platform, build_phase)
        
        return BuildReport(
            platform=platform,
            timestamp=datetime.now().isoformat(),
            build_phase=build_phase,
            success=success,
            errors=errors,
            warnings=warnings,
            summary=summary
        )
    
    def _is_error_line(self, line: str) -> bool:
        """Check if line contains an error"""
        error_indicators = [
            'error:', 'Error:', 'ERROR:', 'fatal error:', 'FATAL ERROR:',
            'configure: error:', 'make: ***', 'failed', 'Failed', 'FAILED'
        ]
        return any(indicator in line for indicator in error_indicators)
    
    def _is_warning_line(self, line: str) -> bool:
        """Check if line contains a warning"""
        warning_indicators = [
            'warning:', 'Warning:', 'WARNING:', 'deprecated', 'Deprecated'
        ]
        return any(indicator in line for indicator in warning_indicators)
    
    def _parse_error_line(self, line: str, line_number: int) -> Optional[BuildError]:
        """Parse error line and categorize"""
        for pattern, (category, severity, description) in self.error_patterns.items():
            if re.search(pattern, line, re.IGNORECASE):
                # Extract file path if present
                file_match = re.search(r'([^:\s]+\.(cpp|c|h|hpp|cc)):', line)
                file_path = file_match.group(1) if file_match else None
                
                # Extract line number if present
                line_match = re.search(r':(\d+):', line)
                error_line = int(line_match.group(1)) if line_match else None
                
                # Get suggestion
                suggestion = self._get_suggestion(category, line)
                
                return BuildError(
                    category=category,
                    severity=severity,
                    message=line.strip(),
                    context=description,
                    file_path=file_path,
                    line_number=error_line,
                    suggestion=suggestion,
                    documentation_link=self._get_documentation_link(category)
                )
        
        # Default unknown error
        return BuildError(
            category=ErrorCategory.UNKNOWN,
            severity=ErrorSeverity.MEDIUM,
            message=line.strip(),
            context="Unrecognized error pattern",
            suggestion="Check the full build log for more context"
        )
    
    def _parse_warning_line(self, line: str, line_number: int) -> Optional[BuildError]:
        """Parse warning line"""
        return BuildError(
            category=ErrorCategory.COMPILATION,
            severity=ErrorSeverity.LOW,
            message=line.strip(),
            context="Compiler warning",
            suggestion="Consider fixing warnings to improve code quality"
        )
    
    def _get_suggestion(self, category: ErrorCategory, line: str) -> str:
        """Get suggestion based on error category and content"""
        suggestions = self.suggestion_map.get(category, {})
        
        # Try to match specific suggestions
        if "not found" in line.lower():
            return suggestions.get("missing_package", "Install missing dependencies")
        elif "undefined reference" in line.lower():
            return suggestions.get("undefined_reference", "Check library linking")
        elif "configure: error" in line.lower():
            return suggestions.get("configure_error", "Check configuration requirements")
        
        # Return generic suggestion for category
        return list(suggestions.values())[0] if suggestions else "Check build requirements"
    
    def _get_documentation_link(self, category: ErrorCategory) -> str:
        """Get documentation link for error category"""
        base_url = "https://github.com/Baba-Chain/BabaChain/blob/main/doc"
        
        links = {
            ErrorCategory.DEPENDENCY: f"{base_url}/dependencies.md",
            ErrorCategory.COMPILATION: f"{base_url}/build-unix.md",
            ErrorCategory.LINKING: f"{base_url}/build-unix.md",
            ErrorCategory.CONFIGURATION: f"{base_url}/build-unix.md",
            ErrorCategory.PLATFORM: f"{base_url}/build-cross-platform.md",
            ErrorCategory.NETWORK: f"{base_url}/build-troubleshooting.md"
        }
        
        return links.get(category, f"{base_url}/README.md")
    
    def _generate_summary(self, errors: List[BuildError], warnings: List[BuildError], 
                         platform: str, build_phase: str) -> str:
        """Generate human-readable summary"""
        if not errors:
            return f"✅ Build successful for {platform} during {build_phase} phase"
        
        error_count = len(errors)
        warning_count = len(warnings)
        
        # Categorize errors
        error_categories = {}
        for error in errors:
            category = error.category.value
            error_categories[category] = error_categories.get(category, 0) + 1
        
        summary = f"❌ Build failed for {platform} during {build_phase} phase\n"
        summary += f"Found {error_count} error(s) and {warning_count} warning(s)\n\n"
        
        # Most common error categories
        if error_categories:
            summary += "Error breakdown:\n"
            for category, count in sorted(error_categories.items(), key=lambda x: x[1], reverse=True):
                summary += f"  • {category}: {count} error(s)\n"
        
        # Top suggestions
        critical_errors = [e for e in errors if e.severity == ErrorSeverity.CRITICAL]
        if critical_errors:
            summary += f"\n🔥 Critical issues to fix first:\n"
            for error in critical_errors[:3]:  # Show top 3
                summary += f"  • {error.context}: {error.suggestion}\n"
        
        return summary
    
    def generate_github_annotation(self, error: BuildError) -> str:
        """Generate GitHub Actions annotation for error"""
        level = "error" if error.severity in [ErrorSeverity.CRITICAL, ErrorSeverity.HIGH] else "warning"
        
        annotation = f"::{level}"
        
        if error.file_path:
            annotation += f" file={error.file_path}"
            if error.line_number:
                annotation += f",line={error.line_number}"
        
        annotation += f"::{error.context} - {error.message}"
        
        return annotation
    
    def export_json(self, report: BuildReport, output_file: str):
        """Export report as JSON"""
        # Convert dataclasses to dict for JSON serialization
        report_dict = asdict(report)
        
        with open(output_file, 'w') as f:
            json.dump(report_dict, f, indent=2, default=str)
    
    def export_markdown(self, report: BuildReport, output_file: str):
        """Export report as Markdown"""
        with open(output_file, 'w') as f:
            f.write(f"# Build Report: {report.platform}\n\n")
            f.write(f"**Timestamp:** {report.timestamp}\n")
            f.write(f"**Build Phase:** {report.build_phase}\n")
            f.write(f"**Status:** {'✅ Success' if report.success else '❌ Failed'}\n\n")
            
            if report.errors:
                f.write(f"## ❌ Errors ({len(report.errors)})\n\n")
                for i, error in enumerate(report.errors, 1):
                    f.write(f"### Error {i}: {error.context}\n\n")
                    f.write(f"**Category:** {error.category.value}\n")
                    f.write(f"**Severity:** {error.severity.value}\n")
                    f.write(f"**Message:** `{error.message}`\n")
                    if error.file_path:
                        f.write(f"**File:** {error.file_path}")
                        if error.line_number:
                            f.write(f":{error.line_number}")
                        f.write("\n")
                    f.write(f"**Suggestion:** {error.suggestion}\n")
                    if error.documentation_link:
                        f.write(f"**Documentation:** [Link]({error.documentation_link})\n")
                    f.write("\n")
            
            if report.warnings:
                f.write(f"## ⚠️ Warnings ({len(report.warnings)})\n\n")
                for i, warning in enumerate(report.warnings[:10], 1):  # Limit to 10 warnings
                    f.write(f"{i}. {warning.message}\n")
                f.write("\n")
            
            f.write(f"## Summary\n\n{report.summary}\n")

def main():
    if len(sys.argv) < 4:
        print("Usage: build-error-analyzer.py <log_file> <platform> <build_phase> [output_format]")
        print("Output formats: json, markdown, github (default: github)")
        sys.exit(1)
    
    log_file = sys.argv[1]
    platform = sys.argv[2]
    build_phase = sys.argv[3]
    output_format = sys.argv[4] if len(sys.argv) > 4 else "github"
    
    if not os.path.exists(log_file):
        print(f"Error: Log file {log_file} not found")
        sys.exit(1)
    
    # Read log file
    with open(log_file, 'r', encoding='utf-8', errors='ignore') as f:
        log_content = f.read()
    
    # Analyze log
    analyzer = BuildErrorAnalyzer()
    report = analyzer.analyze_log(log_content, platform, build_phase)
    
    # Output based on format
    if output_format == "json":
        analyzer.export_json(report, f"build-report-{platform}.json")
        print(f"JSON report saved to build-report-{platform}.json")
    
    elif output_format == "markdown":
        analyzer.export_markdown(report, f"build-report-{platform}.md")
        print(f"Markdown report saved to build-report-{platform}.md")
    
    elif output_format == "github":
        # Output GitHub Actions annotations
        for error in report.errors:
            print(analyzer.generate_github_annotation(error))
        
        # Output summary
        print(f"\n## Build Summary\n{report.summary}")
        
        # Set GitHub Actions outputs
        if os.getenv('GITHUB_ACTIONS'):
            with open(os.environ['GITHUB_OUTPUT'], 'a') as f:
                f.write(f"build_success={'true' if report.success else 'false'}\n")
                f.write(f"error_count={len(report.errors)}\n")
                f.write(f"warning_count={len(report.warnings)}\n")
                f.write(f"build_summary={report.summary.replace('\n', '\\n')}\n")
    
    # Exit with error code if build failed
    sys.exit(0 if report.success else 1)

if __name__ == "__main__":
    main()