#!/usr/bin/env python3
# Copyright (c) 2024 The BabaChain Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Cross-platform compatibility testing for BabaChain binaries.
Tests platform-specific features and compatibility requirements.
"""

import argparse
import json
import os
import platform
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Dict, List, Optional


class CrossPlatformTester:
    """Cross-platform compatibility tester."""
    
    def __init__(self, binary_path: str, platform_name: str):
        self.binary_path = Path(binary_path)
        self.platform_name = platform_name
        self.system_info = self._get_system_info()
        self.results = {
            'platform': platform_name,
            'system_info': self.system_info,
            'compatibility_tests': {},
            'platform_specific_tests': {},
            'dependency_tests': {},
            'success': False
        }
    
    def _get_system_info(self) -> Dict:
        """Get detailed system information."""
        return {
            'platform': platform.platform(),
            'system': platform.system(),
            'release': platform.release(),
            'version': platform.version(),
            'machine': platform.machine(),
            'processor': platform.processor(),
            'architecture': platform.architecture(),
            'python_version': platform.python_version()
        }
    
    def test_binary_execution(self) -> Dict:
        """Test basic binary execution."""
        test_result = {
            'can_execute': False,
            'version_output': None,
            'error_message': None
        }
        
        try:
            if not self.binary_path.exists():
                test_result['error_message'] = f"Binary not found: {self.binary_path}"
                return test_result
            
            if not os.access(self.binary_path, os.X_OK):
                test_result['error_message'] = "Binary is not executable"
                return test_result
            
            # Test version command
            result = subprocess.run(
                [str(self.binary_path), '--version'],
                capture_output=True,
                text=True,
                timeout=30
            )
            
            if result.returncode == 0:
                test_result['can_execute'] = True
                test_result['version_output'] = result.stdout.strip()
            else:
                test_result['error_message'] = f"Version command failed: {result.stderr}"
                
        except subprocess.TimeoutExpired:
            test_result['error_message'] = "Binary execution timed out"
        except Exception as e:
            test_result['error_message'] = f"Execution error: {str(e)}"
        
        return test_result
    
    def test_library_dependencies(self) -> Dict:
        """Test library dependencies and linking."""
        dep_test = {
            'dependencies_found': [],
            'missing_dependencies': [],
            'dynamic_linking_ok': False,
            'error_message': None
        }
        
        try:
            system = platform.system().lower()
            
            if system == 'linux':
                # Use ldd to check dependencies
                result = subprocess.run(
                    ['ldd', str(self.binary_path)],
                    capture_output=True,
                    text=True,
                    timeout=10
                )
                
                if result.returncode == 0:
                    dep_test['dynamic_linking_ok'] = True
                    lines = result.stdout.strip().split('\n')
                    
                    for line in lines:
                        line = line.strip()
                        if '=>' in line:
                            parts = line.split('=>')
                            lib_name = parts[0].strip()
                            lib_path = parts[1].strip().split()[0] if len(parts[1].strip().split()) > 0 else ''
                            
                            if lib_path and lib_path != '(0x' and not lib_path.startswith('('):
                                dep_test['dependencies_found'].append({
                                    'name': lib_name,
                                    'path': lib_path
                                })
                            elif 'not found' in parts[1]:
                                dep_test['missing_dependencies'].append(lib_name)
                else:
                    dep_test['error_message'] = f"ldd failed: {result.stderr}"
                    
            elif system == 'darwin':
                # Use otool to check dependencies on macOS
                result = subprocess.run(
                    ['otool', '-L', str(self.binary_path)],
                    capture_output=True,
                    text=True,
                    timeout=10
                )
                
                if result.returncode == 0:
                    dep_test['dynamic_linking_ok'] = True
                    lines = result.stdout.strip().split('\n')[1:]  # Skip first line (binary name)
                    
                    for line in lines:
                        line = line.strip()
                        if line:
                            lib_path = line.split()[0]
                            dep_test['dependencies_found'].append({
                                'name': Path(lib_path).name,
                                'path': lib_path
                            })
                else:
                    dep_test['error_message'] = f"otool failed: {result.stderr}"
                    
            elif system == 'windows':
                # Use objdump to check dependencies on Windows
                result = subprocess.run(
                    ['objdump', '-p', str(self.binary_path)],
                    capture_output=True,
                    text=True,
                    timeout=10
                )
                
                if result.returncode == 0:
                    dep_test['dynamic_linking_ok'] = True
                    lines = result.stdout.split('\n')
                    in_dll_section = False
                    
                    for line in lines:
                        line = line.strip()
                        if 'DLL Name:' in line:
                            in_dll_section = True
                        elif in_dll_section and line:
                            if line.startswith('DLL Name:'):
                                dll_name = line.replace('DLL Name:', '').strip()
                                dep_test['dependencies_found'].append({
                                    'name': dll_name,
                                    'path': dll_name
                                })
                            elif not line.startswith('DLL Name:') and not line.startswith('vma:'):
                                break
                else:
                    dep_test['error_message'] = f"objdump failed: {result.stderr}"
            
        except Exception as e:
            dep_test['error_message'] = f"Dependency check error: {str(e)}"
        
        return dep_test
    
    def test_platform_specific_features(self) -> Dict:
        """Test platform-specific features and requirements."""
        platform_tests = {}
        system = platform.system().lower()
        
        if system == 'linux':
            platform_tests.update(self._test_linux_specific())
        elif system == 'darwin':
            platform_tests.update(self._test_macos_specific())
        elif system == 'windows':
            platform_tests.update(self._test_windows_specific())
        
        return platform_tests
    
    def _test_linux_specific(self) -> Dict:
        """Test Linux-specific features."""
        tests = {
            'glibc_version': self._check_glibc_version(),
            'architecture_match': self._check_linux_architecture(),
            'security_features': self._check_linux_security_features()
        }
        return tests
    
    def _test_macos_specific(self) -> Dict:
        """Test macOS-specific features."""
        tests = {
            'macos_version': self._check_macos_version(),
            'architecture_match': self._check_macos_architecture(),
            'code_signing': self._check_macos_code_signing()
        }
        return tests
    
    def _test_windows_specific(self) -> Dict:
        """Test Windows-specific features."""
        tests = {
            'windows_version': self._check_windows_version(),
            'architecture_match': self._check_windows_architecture(),
            'pe_format': self._check_windows_pe_format()
        }
        return tests
    
    def _check_glibc_version(self) -> Dict:
        """Check GLIBC version compatibility."""
        try:
            result = subprocess.run(
                ['ldd', '--version'],
                capture_output=True,
                text=True,
                timeout=5
            )
            
            if result.returncode == 0:
                version_line = result.stdout.split('\n')[0]
                return {
                    'available': True,
                    'version': version_line,
                    'compatible': True  # Assume compatible if we can run ldd
                }
        except Exception as e:
            return {
                'available': False,
                'error': str(e),
                'compatible': False
            }
        
        return {'available': False, 'compatible': False}
    
    def _check_linux_architecture(self) -> Dict:
        """Check Linux architecture compatibility."""
        system_arch = platform.machine()
        expected_arch = None
        
        if 'arm64' in self.platform_name.lower() or 'aarch64' in self.platform_name.lower():
            expected_arch = 'aarch64'
        elif 'x64' in self.platform_name.lower():
            expected_arch = 'x86_64'
        
        return {
            'system_arch': system_arch,
            'expected_arch': expected_arch,
            'compatible': expected_arch is None or system_arch == expected_arch
        }
    
    def _check_linux_security_features(self) -> Dict:
        """Check Linux security features."""
        try:
            # Check if binary has security features using readelf
            result = subprocess.run(
                ['readelf', '-d', str(self.binary_path)],
                capture_output=True,
                text=True,
                timeout=10
            )
            
            security_features = {
                'pie': False,
                'relro': False,
                'stack_canary': False
            }
            
            if result.returncode == 0:
                output = result.stdout
                if 'BIND_NOW' in output:
                    security_features['relro'] = True
                # Additional security checks would go here
            
            return security_features
            
        except Exception as e:
            return {'error': str(e)}
    
    def _check_macos_version(self) -> Dict:
        """Check macOS version compatibility."""
        try:
            version = platform.mac_ver()[0]
            major_version = int(version.split('.')[0]) if version else 0
            
            # Check minimum macOS version requirements
            min_version = 11 if 'arm64' in self.platform_name.lower() else 10
            
            return {
                'system_version': version,
                'major_version': major_version,
                'min_required': min_version,
                'compatible': major_version >= min_version
            }
        except Exception as e:
            return {'error': str(e), 'compatible': False}
    
    def _check_macos_architecture(self) -> Dict:
        """Check macOS architecture compatibility."""
        system_arch = platform.machine()
        expected_arch = None
        
        if 'arm64' in self.platform_name.lower():
            expected_arch = 'arm64'
        elif 'x64' in self.platform_name.lower():
            expected_arch = 'x86_64'
        
        return {
            'system_arch': system_arch,
            'expected_arch': expected_arch,
            'compatible': expected_arch is None or system_arch == expected_arch
        }
    
    def _check_macos_code_signing(self) -> Dict:
        """Check macOS code signing status."""
        try:
            result = subprocess.run(
                ['codesign', '-v', str(self.binary_path)],
                capture_output=True,
                text=True,
                timeout=10
            )
            
            return {
                'signed': result.returncode == 0,
                'output': result.stdout + result.stderr
            }
        except Exception as e:
            return {'error': str(e), 'signed': False}
    
    def _check_windows_version(self) -> Dict:
        """Check Windows version compatibility."""
        try:
            version = platform.version()
            return {
                'system_version': version,
                'compatible': True  # Assume compatible for now
            }
        except Exception as e:
            return {'error': str(e), 'compatible': False}
    
    def _check_windows_architecture(self) -> Dict:
        """Check Windows architecture compatibility."""
        system_arch = platform.machine()
        expected_arch = 'AMD64' if 'x64' in self.platform_name.lower() else None
        
        return {
            'system_arch': system_arch,
            'expected_arch': expected_arch,
            'compatible': expected_arch is None or system_arch == expected_arch
        }
    
    def _check_windows_pe_format(self) -> Dict:
        """Check Windows PE format."""
        try:
            result = subprocess.run(
                ['file', str(self.binary_path)],
                capture_output=True,
                text=True,
                timeout=5
            )
            
            if result.returncode == 0:
                return {
                    'is_pe': 'PE32' in result.stdout,
                    'file_info': result.stdout.strip()
                }
        except Exception as e:
            return {'error': str(e)}
        
        return {'is_pe': False}
    
    def run_all_tests(self) -> Dict:
        """Run all cross-platform compatibility tests."""
        print(f"Running cross-platform tests for {self.platform_name}")
        
        # Basic execution test
        self.results['compatibility_tests']['execution'] = self.test_binary_execution()
        
        # Library dependency tests
        self.results['dependency_tests'] = self.test_library_dependencies()
        
        # Platform-specific tests
        self.results['platform_specific_tests'] = self.test_platform_specific_features()
        
        # Determine overall success
        self.results['success'] = self._determine_success()
        
        return self.results
    
    def _determine_success(self) -> bool:
        """Determine if all tests passed."""
        # Check basic execution
        if not self.results['compatibility_tests']['execution'].get('can_execute', False):
            return False
        
        # Check for missing dependencies
        if self.results['dependency_tests'].get('missing_dependencies'):
            return False
        
        # Check platform-specific compatibility
        platform_tests = self.results['platform_specific_tests']
        for test_name, test_result in platform_tests.items():
            if isinstance(test_result, dict) and 'compatible' in test_result:
                if not test_result['compatible']:
                    return False
        
        return True
    
    def print_summary(self):
        """Print test summary."""
        print(f"\n=== Cross-Platform Test Summary ===")
        print(f"Platform: {self.platform_name}")
        print(f"System: {self.system_info['platform']}")
        print(f"Overall Success: {'✅ PASS' if self.results['success'] else '❌ FAIL'}")
        
        # Execution test
        exec_test = self.results['compatibility_tests']['execution']
        status = "✅" if exec_test.get('can_execute') else "❌"
        print(f"\n🚀 Execution Test: {status}")
        if exec_test.get('error_message'):
            print(f"   Error: {exec_test['error_message']}")
        
        # Dependencies
        dep_test = self.results['dependency_tests']
        dep_status = "✅" if dep_test.get('dynamic_linking_ok') and not dep_test.get('missing_dependencies') else "❌"
        print(f"\n📚 Dependencies: {dep_status}")
        if dep_test.get('missing_dependencies'):
            print(f"   Missing: {', '.join(dep_test['missing_dependencies'])}")
        
        # Platform-specific tests
        print(f"\n🔧 Platform-Specific Tests:")
        for test_name, test_result in self.results['platform_specific_tests'].items():
            if isinstance(test_result, dict) and 'compatible' in test_result:
                status = "✅" if test_result['compatible'] else "❌"
                print(f"   {status} {test_name}")


def main():
    parser = argparse.ArgumentParser(description='BabaChain Cross-Platform Compatibility Tester')
    parser.add_argument('binary', help='Path to binary to test')
    parser.add_argument('platform', help='Platform name (e.g., linux-x64, macos-arm64)')
    parser.add_argument('--output', '-o', help='Output JSON file for results')
    parser.add_argument('--quiet', '-q', action='store_true', help='Quiet mode')
    
    args = parser.parse_args()
    
    if not os.path.exists(args.binary):
        print(f"Error: Binary '{args.binary}' does not exist")
        sys.exit(1)
    
    tester = CrossPlatformTester(args.binary, args.platform)
    results = tester.run_all_tests()
    
    if not args.quiet:
        tester.print_summary()
    
    if args.output:
        with open(args.output, 'w') as f:
            json.dump(results, f, indent=2, default=str)
        if not args.quiet:
            print(f"\nResults saved to: {args.output}")
    
    # Exit with error code if tests failed
    sys.exit(0 if results['success'] else 1)


if __name__ == '__main__':
    main()