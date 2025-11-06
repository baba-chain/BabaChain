#!/usr/bin/env python3
"""
BabaChain Build Artifact Validator
Validates build artifacts for functionality, security, and compatibility
"""

import os
import sys
import json
import hashlib
import subprocess
import tempfile
import shutil
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass, asdict
from enum import Enum
import platform as platform_module

class ValidationResult(Enum):
    PASS = "pass"
    FAIL = "fail"
    WARNING = "warning"
    SKIP = "skip"

class ValidationCategory(Enum):
    FUNCTIONALITY = "functionality"
    SECURITY = "security"
    COMPATIBILITY = "compatibility"
    PERFORMANCE = "performance"
    METADATA = "metadata"

@dataclass
class ValidationTest:
    name: str
    category: ValidationCategory
    description: str
    result: ValidationResult
    message: str
    details: Optional[Dict] = None

@dataclass
class ArtifactMetadata:
    name: str
    version: str
    platform: str
    architecture: str
    build_timestamp: str
    git_commit: str
    git_branch: str
    file_size: int
    checksum_sha256: str
    checksum_md5: str
    dependencies: List[str]
    build_flags: List[str]

@dataclass
class ValidationReport:
    artifact_path: str
    metadata: ArtifactMetadata
    tests: List[ValidationTest]
    summary: Dict[ValidationResult, int]
    overall_result: ValidationResult
    timestamp: str

class ArtifactValidator:
    def __init__(self, artifact_path: str):
        self.artifact_path = Path(artifact_path)
        self.platform = self._detect_platform()
        self.tests = []
        
    def _detect_platform(self) -> str:
        """Detect current platform"""
        system = platform_module.system().lower()
        machine = platform_module.machine().lower()
        
        if system == "linux":
            if machine in ["aarch64", "arm64"]:
                return "linux-arm64"
            else:
                return "linux-x64"
        elif system == "darwin":
            if machine in ["arm64"]:
                return "macos-arm64"
            else:
                return "macos-x64"
        elif system == "windows":
            return "windows-x64"
        else:
            return f"{system}-{machine}"
    
    def validate(self) -> ValidationReport:
        """Run all validation tests"""
        if not self.artifact_path.exists():
            raise FileNotFoundError(f"Artifact not found: {self.artifact_path}")
        
        # Generate metadata
        metadata = self._generate_metadata()
        
        # Run validation tests
        self._test_file_integrity()
        self._test_binary_functionality()
        self._test_security_features()
        self._test_platform_compatibility()
        self._test_performance_characteristics()
        
        # Generate summary
        summary = self._generate_summary()
        overall_result = self._determine_overall_result()
        
        return ValidationReport(
            artifact_path=str(self.artifact_path),
            metadata=metadata,
            tests=self.tests,
            summary=summary,
            overall_result=overall_result,
            timestamp=self._get_timestamp()
        )
    
    def _generate_metadata(self) -> ArtifactMetadata:
        """Generate artifact metadata"""
        file_size = self.artifact_path.stat().st_size
        
        # Calculate checksums
        sha256_hash = self._calculate_checksum(self.artifact_path, 'sha256')
        md5_hash = self._calculate_checksum(self.artifact_path, 'md5')
        
        # Get version from binary if possible
        version = self._extract_version()
        
        # Get build information
        git_commit = os.getenv('GITHUB_SHA', self._get_git_commit())
        git_branch = os.getenv('GITHUB_REF_NAME', self._get_git_branch())
        
        # Detect dependencies
        dependencies = self._detect_dependencies()
        
        return ArtifactMetadata(
            name=self.artifact_path.name,
            version=version,
            platform=self.platform,
            architecture=platform_module.machine(),
            build_timestamp=self._get_timestamp(),
            git_commit=git_commit,
            git_branch=git_branch,
            file_size=file_size,
            checksum_sha256=sha256_hash,
            checksum_md5=md5_hash,
            dependencies=dependencies,
            build_flags=self._extract_build_flags()
        )
    
    def _calculate_checksum(self, file_path: Path, algorithm: str) -> str:
        """Calculate file checksum"""
        hash_func = hashlib.new(algorithm)
        
        with open(file_path, 'rb') as f:
            for chunk in iter(lambda: f.read(4096), b""):
                hash_func.update(chunk)
        
        return hash_func.hexdigest()
    
    def _extract_version(self) -> str:
        """Extract version from binary"""
        try:
            result = subprocess.run(
                [str(self.artifact_path), '--version'],
                capture_output=True,
                text=True,
                timeout=10
            )
            if result.returncode == 0:
                # Extract version from output
                lines = result.stdout.strip().split('\n')
                for line in lines:
                    if 'version' in line.lower():
                        # Try to extract version number
                        import re
                        version_match = re.search(r'(\d+\.\d+\.\d+)', line)
                        if version_match:
                            return version_match.group(1)
                return lines[0] if lines else "unknown"
        except Exception:
            pass
        
        return "unknown"
    
    def _get_git_commit(self) -> str:
        """Get current git commit"""
        try:
            result = subprocess.run(
                ['git', 'rev-parse', 'HEAD'],
                capture_output=True,
                text=True,
                timeout=5
            )
            if result.returncode == 0:
                return result.stdout.strip()
        except Exception:
            pass
        return "unknown"
    
    def _get_git_branch(self) -> str:
        """Get current git branch"""
        try:
            result = subprocess.run(
                ['git', 'rev-parse', '--abbrev-ref', 'HEAD'],
                capture_output=True,
                text=True,
                timeout=5
            )
            if result.returncode == 0:
                return result.stdout.strip()
        except Exception:
            pass
        return "unknown"
    
    def _detect_dependencies(self) -> List[str]:
        """Detect binary dependencies"""
        dependencies = []
        
        try:
            if self.platform.startswith('linux'):
                # Use ldd for Linux
                result = subprocess.run(
                    ['ldd', str(self.artifact_path)],
                    capture_output=True,
                    text=True,
                    timeout=10
                )
                if result.returncode == 0:
                    for line in result.stdout.split('\n'):
                        if '=>' in line:
                            lib = line.split('=>')[0].strip()
                            if lib:
                                dependencies.append(lib)
            
            elif self.platform.startswith('macos'):
                # Use otool for macOS
                result = subprocess.run(
                    ['otool', '-L', str(self.artifact_path)],
                    capture_output=True,
                    text=True,
                    timeout=10
                )
                if result.returncode == 0:
                    lines = result.stdout.split('\n')[1:]  # Skip first line
                    for line in lines:
                        if line.strip():
                            lib = line.strip().split()[0]
                            dependencies.append(lib)
            
            elif self.platform.startswith('windows'):
                # Use objdump for Windows
                result = subprocess.run(
                    ['objdump', '-p', str(self.artifact_path)],
                    capture_output=True,
                    text=True,
                    timeout=10
                )
                if result.returncode == 0:
                    for line in result.stdout.split('\n'):
                        if 'DLL Name:' in line:
                            dll = line.split('DLL Name:')[1].strip()
                            dependencies.append(dll)
        
        except Exception:
            pass
        
        return dependencies[:20]  # Limit to first 20 dependencies
    
    def _extract_build_flags(self) -> List[str]:
        """Extract build flags from binary (if available)"""
        # This is a placeholder - actual implementation would depend on
        # how build flags are embedded in the binary
        return []
    
    def _get_timestamp(self) -> str:
        """Get current timestamp"""
        from datetime import datetime
        return datetime.now().isoformat()
    
    def _test_file_integrity(self):
        """Test file integrity"""
        # Check if file exists and is readable
        if not self.artifact_path.exists():
            self.tests.append(ValidationTest(
                name="file_exists",
                category=ValidationCategory.METADATA,
                description="Check if artifact file exists",
                result=ValidationResult.FAIL,
                message="Artifact file does not exist"
            ))
            return
        
        self.tests.append(ValidationTest(
            name="file_exists",
            category=ValidationCategory.METADATA,
            description="Check if artifact file exists",
            result=ValidationResult.PASS,
            message="Artifact file exists and is readable"
        ))
        
        # Check file size
        file_size = self.artifact_path.stat().st_size
        if file_size == 0:
            self.tests.append(ValidationTest(
                name="file_size",
                category=ValidationCategory.METADATA,
                description="Check if artifact file has content",
                result=ValidationResult.FAIL,
                message="Artifact file is empty"
            ))
        elif file_size < 1024:  # Less than 1KB
            self.tests.append(ValidationTest(
                name="file_size",
                category=ValidationCategory.METADATA,
                description="Check if artifact file has reasonable size",
                result=ValidationResult.WARNING,
                message=f"Artifact file is very small ({file_size} bytes)"
            ))
        else:
            self.tests.append(ValidationTest(
                name="file_size",
                category=ValidationCategory.METADATA,
                description="Check if artifact file has reasonable size",
                result=ValidationResult.PASS,
                message=f"Artifact file size is reasonable ({file_size} bytes)"
            ))
        
        # Check if file is executable
        if not os.access(self.artifact_path, os.X_OK):
            self.tests.append(ValidationTest(
                name="executable_permissions",
                category=ValidationCategory.FUNCTIONALITY,
                description="Check if artifact has executable permissions",
                result=ValidationResult.FAIL,
                message="Artifact file is not executable"
            ))
        else:
            self.tests.append(ValidationTest(
                name="executable_permissions",
                category=ValidationCategory.FUNCTIONALITY,
                description="Check if artifact has executable permissions",
                result=ValidationResult.PASS,
                message="Artifact file has executable permissions"
            ))
    
    def _test_binary_functionality(self):
        """Test basic binary functionality"""
        # Test version command
        try:
            result = subprocess.run(
                [str(self.artifact_path), '--version'],
                capture_output=True,
                text=True,
                timeout=30
            )
            
            if result.returncode == 0:
                self.tests.append(ValidationTest(
                    name="version_command",
                    category=ValidationCategory.FUNCTIONALITY,
                    description="Test --version command",
                    result=ValidationResult.PASS,
                    message="Version command executed successfully",
                    details={"output": result.stdout.strip()}
                ))
            else:
                self.tests.append(ValidationTest(
                    name="version_command",
                    category=ValidationCategory.FUNCTIONALITY,
                    description="Test --version command",
                    result=ValidationResult.FAIL,
                    message=f"Version command failed with exit code {result.returncode}",
                    details={"stderr": result.stderr.strip()}
                ))
        
        except subprocess.TimeoutExpired:
            self.tests.append(ValidationTest(
                name="version_command",
                category=ValidationCategory.FUNCTIONALITY,
                description="Test --version command",
                result=ValidationResult.FAIL,
                message="Version command timed out"
            ))
        except Exception as e:
            self.tests.append(ValidationTest(
                name="version_command",
                category=ValidationCategory.FUNCTIONALITY,
                description="Test --version command",
                result=ValidationResult.FAIL,
                message=f"Version command failed: {e}"
            ))
        
        # Test help command
        try:
            result = subprocess.run(
                [str(self.artifact_path), '--help'],
                capture_output=True,
                text=True,
                timeout=30
            )
            
            if result.returncode == 0:
                self.tests.append(ValidationTest(
                    name="help_command",
                    category=ValidationCategory.FUNCTIONALITY,
                    description="Test --help command",
                    result=ValidationResult.PASS,
                    message="Help command executed successfully"
                ))
            else:
                self.tests.append(ValidationTest(
                    name="help_command",
                    category=ValidationCategory.FUNCTIONALITY,
                    description="Test --help command",
                    result=ValidationResult.WARNING,
                    message=f"Help command returned exit code {result.returncode}"
                ))
        
        except Exception as e:
            self.tests.append(ValidationTest(
                name="help_command",
                category=ValidationCategory.FUNCTIONALITY,
                description="Test --help command",
                result=ValidationResult.WARNING,
                message=f"Help command failed: {e}"
            ))
    
    def _test_security_features(self):
        """Test security features"""
        # Check for stack protection (if available)
        self._check_stack_protection()
        
        # Check for ASLR support
        self._check_aslr_support()
        
        # Check for stripped symbols
        self._check_stripped_symbols()
    
    def _check_stack_protection(self):
        """Check for stack protection features"""
        try:
            if self.platform.startswith('linux'):
                result = subprocess.run(
                    ['readelf', '-s', str(self.artifact_path)],
                    capture_output=True,
                    text=True,
                    timeout=10
                )
                
                if result.returncode == 0:
                    if '__stack_chk_fail' in result.stdout:
                        self.tests.append(ValidationTest(
                            name="stack_protection",
                            category=ValidationCategory.SECURITY,
                            description="Check for stack protection",
                            result=ValidationResult.PASS,
                            message="Stack protection is enabled"
                        ))
                    else:
                        self.tests.append(ValidationTest(
                            name="stack_protection",
                            category=ValidationCategory.SECURITY,
                            description="Check for stack protection",
                            result=ValidationResult.WARNING,
                            message="Stack protection not detected"
                        ))
                else:
                    self.tests.append(ValidationTest(
                        name="stack_protection",
                        category=ValidationCategory.SECURITY,
                        description="Check for stack protection",
                        result=ValidationResult.SKIP,
                        message="Could not check stack protection"
                    ))
            else:
                self.tests.append(ValidationTest(
                    name="stack_protection",
                    category=ValidationCategory.SECURITY,
                    description="Check for stack protection",
                    result=ValidationResult.SKIP,
                    message="Stack protection check not implemented for this platform"
                ))
        
        except Exception:
            self.tests.append(ValidationTest(
                name="stack_protection",
                category=ValidationCategory.SECURITY,
                description="Check for stack protection",
                result=ValidationResult.SKIP,
                message="Stack protection check failed"
            ))
    
    def _check_aslr_support(self):
        """Check for ASLR support"""
        try:
            if self.platform.startswith('linux'):
                result = subprocess.run(
                    ['readelf', '-h', str(self.artifact_path)],
                    capture_output=True,
                    text=True,
                    timeout=10
                )
                
                if result.returncode == 0:
                    if 'DYN' in result.stdout:
                        self.tests.append(ValidationTest(
                            name="aslr_support",
                            category=ValidationCategory.SECURITY,
                            description="Check for ASLR support",
                            result=ValidationResult.PASS,
                            message="Binary supports ASLR (Position Independent Executable)"
                        ))
                    else:
                        self.tests.append(ValidationTest(
                            name="aslr_support",
                            category=ValidationCategory.SECURITY,
                            description="Check for ASLR support",
                            result=ValidationResult.WARNING,
                            message="Binary may not support ASLR"
                        ))
                else:
                    self.tests.append(ValidationTest(
                        name="aslr_support",
                        category=ValidationCategory.SECURITY,
                        description="Check for ASLR support",
                        result=ValidationResult.SKIP,
                        message="Could not check ASLR support"
                    ))
            else:
                self.tests.append(ValidationTest(
                    name="aslr_support",
                    category=ValidationCategory.SECURITY,
                    description="Check for ASLR support",
                    result=ValidationResult.SKIP,
                    message="ASLR check not implemented for this platform"
                ))
        
        except Exception:
            self.tests.append(ValidationTest(
                name="aslr_support",
                category=ValidationCategory.SECURITY,
                description="Check for ASLR support",
                result=ValidationResult.SKIP,
                message="ASLR check failed"
            ))
    
    def _check_stripped_symbols(self):
        """Check if binary is stripped"""
        try:
            if self.platform.startswith('linux') or self.platform.startswith('macos'):
                cmd = ['file', str(self.artifact_path)]
                result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
                
                if result.returncode == 0:
                    if 'stripped' in result.stdout.lower():
                        self.tests.append(ValidationTest(
                            name="stripped_symbols",
                            category=ValidationCategory.SECURITY,
                            description="Check if binary is stripped",
                            result=ValidationResult.PASS,
                            message="Binary is stripped (symbols removed)"
                        ))
                    else:
                        self.tests.append(ValidationTest(
                            name="stripped_symbols",
                            category=ValidationCategory.SECURITY,
                            description="Check if binary is stripped",
                            result=ValidationResult.WARNING,
                            message="Binary contains debug symbols"
                        ))
                else:
                    self.tests.append(ValidationTest(
                        name="stripped_symbols",
                        category=ValidationCategory.SECURITY,
                        description="Check if binary is stripped",
                        result=ValidationResult.SKIP,
                        message="Could not check if binary is stripped"
                    ))
            else:
                self.tests.append(ValidationTest(
                    name="stripped_symbols",
                    category=ValidationCategory.SECURITY,
                    description="Check if binary is stripped",
                    result=ValidationResult.SKIP,
                    message="Symbol stripping check not implemented for this platform"
                ))
        
        except Exception:
            self.tests.append(ValidationTest(
                name="stripped_symbols",
                category=ValidationCategory.SECURITY,
                description="Check if binary is stripped",
                result=ValidationResult.SKIP,
                message="Symbol stripping check failed"
            ))
    
    def _test_platform_compatibility(self):
        """Test platform compatibility"""
        # Check architecture compatibility
        self._check_architecture_compatibility()
        
        # Check library dependencies
        self._check_library_dependencies()
    
    def _check_architecture_compatibility(self):
        """Check if binary architecture matches platform"""
        try:
            result = subprocess.run(
                ['file', str(self.artifact_path)],
                capture_output=True,
                text=True,
                timeout=10
            )
            
            if result.returncode == 0:
                file_output = result.stdout.lower()
                
                # Check architecture
                current_arch = platform_module.machine().lower()
                
                arch_compatible = False
                if current_arch in ['x86_64', 'amd64'] and ('x86-64' in file_output or 'x86_64' in file_output):
                    arch_compatible = True
                elif current_arch in ['aarch64', 'arm64'] and ('aarch64' in file_output or 'arm64' in file_output):
                    arch_compatible = True
                elif current_arch in ['arm'] and 'arm' in file_output:
                    arch_compatible = True
                
                if arch_compatible:
                    self.tests.append(ValidationTest(
                        name="architecture_compatibility",
                        category=ValidationCategory.COMPATIBILITY,
                        description="Check architecture compatibility",
                        result=ValidationResult.PASS,
                        message=f"Binary architecture is compatible with {current_arch}"
                    ))
                else:
                    self.tests.append(ValidationTest(
                        name="architecture_compatibility",
                        category=ValidationCategory.COMPATIBILITY,
                        description="Check architecture compatibility",
                        result=ValidationResult.WARNING,
                        message=f"Binary architecture may not be compatible with {current_arch}"
                    ))
            else:
                self.tests.append(ValidationTest(
                    name="architecture_compatibility",
                    category=ValidationCategory.COMPATIBILITY,
                    description="Check architecture compatibility",
                    result=ValidationResult.SKIP,
                    message="Could not determine binary architecture"
                ))
        
        except Exception:
            self.tests.append(ValidationTest(
                name="architecture_compatibility",
                category=ValidationCategory.COMPATIBILITY,
                description="Check architecture compatibility",
                result=ValidationResult.SKIP,
                message="Architecture compatibility check failed"
            ))
    
    def _check_library_dependencies(self):
        """Check library dependencies"""
        dependencies = self._detect_dependencies()
        
        if not dependencies:
            self.tests.append(ValidationTest(
                name="library_dependencies",
                category=ValidationCategory.COMPATIBILITY,
                description="Check library dependencies",
                result=ValidationResult.SKIP,
                message="Could not detect library dependencies"
            ))
            return
        
        # Check for problematic dependencies
        problematic_deps = []
        for dep in dependencies:
            # Check for absolute paths (bad for portability)
            if dep.startswith('/usr/local/') or dep.startswith('/opt/'):
                problematic_deps.append(dep)
        
        if problematic_deps:
            self.tests.append(ValidationTest(
                name="library_dependencies",
                category=ValidationCategory.COMPATIBILITY,
                description="Check library dependencies",
                result=ValidationResult.WARNING,
                message=f"Found {len(problematic_deps)} potentially problematic dependencies",
                details={"problematic_dependencies": problematic_deps}
            ))
        else:
            self.tests.append(ValidationTest(
                name="library_dependencies",
                category=ValidationCategory.COMPATIBILITY,
                description="Check library dependencies",
                result=ValidationResult.PASS,
                message=f"All {len(dependencies)} dependencies look reasonable"
            ))
    
    def _test_performance_characteristics(self):
        """Test performance characteristics"""
        # Check binary size
        file_size = self.artifact_path.stat().st_size
        
        # Define size thresholds (in MB)
        size_mb = file_size / (1024 * 1024)
        
        if size_mb > 100:  # Very large
            result = ValidationResult.WARNING
            message = f"Binary is very large ({size_mb:.1f} MB)"
        elif size_mb > 50:  # Large
            result = ValidationResult.WARNING
            message = f"Binary is large ({size_mb:.1f} MB)"
        elif size_mb < 1:  # Very small
            result = ValidationResult.WARNING
            message = f"Binary is very small ({size_mb:.1f} MB)"
        else:
            result = ValidationResult.PASS
            message = f"Binary size is reasonable ({size_mb:.1f} MB)"
        
        self.tests.append(ValidationTest(
            name="binary_size",
            category=ValidationCategory.PERFORMANCE,
            description="Check binary size",
            result=result,
            message=message,
            details={"size_bytes": file_size, "size_mb": size_mb}
        ))
    
    def _generate_summary(self) -> Dict[ValidationResult, int]:
        """Generate test result summary"""
        summary = {result: 0 for result in ValidationResult}
        
        for test in self.tests:
            summary[test.result] += 1
        
        return summary
    
    def _determine_overall_result(self) -> ValidationResult:
        """Determine overall validation result"""
        summary = self._generate_summary()
        
        if summary[ValidationResult.FAIL] > 0:
            return ValidationResult.FAIL
        elif summary[ValidationResult.WARNING] > 0:
            return ValidationResult.WARNING
        elif summary[ValidationResult.PASS] > 0:
            return ValidationResult.PASS
        else:
            return ValidationResult.SKIP

def main():
    if len(sys.argv) < 2:
        print("Usage: artifact-validator.py <artifact_path> [output_format]")
        print("Output formats: json, markdown, text (default: text)")
        sys.exit(1)
    
    artifact_path = sys.argv[1]
    output_format = sys.argv[2] if len(sys.argv) > 2 else "text"
    
    if not os.path.exists(artifact_path):
        print(f"Error: Artifact not found: {artifact_path}")
        sys.exit(1)
    
    # Validate artifact
    validator = ArtifactValidator(artifact_path)
    report = validator.validate()
    
    # Output based on format
    if output_format == "json":
        print(json.dumps(asdict(report), indent=2, default=str))
    
    elif output_format == "markdown":
        print(f"# Artifact Validation Report\n")
        print(f"**Artifact:** {report.artifact_path}")
        print(f"**Platform:** {report.metadata.platform}")
        print(f"**Overall Result:** {report.overall_result.value.upper()}")
        print(f"**Timestamp:** {report.timestamp}\n")
        
        print(f"## Summary")
        print(f"- ✅ Passed: {report.summary[ValidationResult.PASS]}")
        print(f"- ⚠️ Warnings: {report.summary[ValidationResult.WARNING]}")
        print(f"- ❌ Failed: {report.summary[ValidationResult.FAIL]}")
        print(f"- ⏭️ Skipped: {report.summary[ValidationResult.SKIP]}\n")
        
        print(f"## Test Results")
        for test in report.tests:
            icon = {"pass": "✅", "warning": "⚠️", "fail": "❌", "skip": "⏭️"}[test.result.value]
            print(f"### {icon} {test.name}")
            print(f"**Category:** {test.category.value}")
            print(f"**Description:** {test.description}")
            print(f"**Result:** {test.message}\n")
    
    else:  # text format
        print(f"Artifact Validation Report")
        print(f"=" * 50)
        print(f"Artifact: {report.artifact_path}")
        print(f"Platform: {report.metadata.platform}")
        print(f"Overall Result: {report.overall_result.value.upper()}")
        print(f"Timestamp: {report.timestamp}")
        print()
        
        print(f"Summary:")
        print(f"  Passed: {report.summary[ValidationResult.PASS]}")
        print(f"  Warnings: {report.summary[ValidationResult.WARNING]}")
        print(f"  Failed: {report.summary[ValidationResult.FAIL]}")
        print(f"  Skipped: {report.summary[ValidationResult.SKIP]}")
        print()
        
        print(f"Test Results:")
        for test in report.tests:
            status = test.result.value.upper()
            print(f"  [{status}] {test.name}: {test.message}")
    
    # Exit with error code if validation failed
    sys.exit(0 if report.overall_result != ValidationResult.FAIL else 1)

if __name__ == "__main__":
    main()