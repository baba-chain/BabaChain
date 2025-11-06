#!/usr/bin/env python3
# Copyright (c) 2024 The BabaChain Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Comprehensive build verification script for BabaChain binaries.
Performs functionality tests, compatibility checks, and generates checksums.
"""

import argparse
import hashlib
import json
import os
import platform
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from typing import Dict, List, Optional, Tuple


class BuildVerifier:
    """Main class for build verification operations."""
    
    def __init__(self, binary_dir: str, platform_name: str):
        self.binary_dir = Path(binary_dir)
        self.platform_name = platform_name
        self.results = {
            'platform': platform_name,
            'timestamp': time.time(),
            'tests': {},
            'checksums': {},
            'compatibility': {},
            'performance': {}
        }
        
        # Expected binaries for each platform
        self.expected_binaries = {
            'babachaind': True,  # Required
            'babachain-cli': True,  # Required
            'babachain-qt': False,  # Optional (GUI)
            'babachain-tx': False,  # Optional
            'babachain-wallet': False,  # Optional
        }
    
    def find_binaries(self) -> Dict[str, Optional[Path]]:
        """Find all available binaries in the directory."""
        binaries = {}
        binary_ext = '.exe' if 'windows' in self.platform_name.lower() else ''
        
        for binary_name in self.expected_binaries:
            binary_path = self.binary_dir / f"{binary_name}{binary_ext}"
            if binary_path.exists() and binary_path.is_file():
                binaries[binary_name] = binary_path
            else:
                binaries[binary_name] = None
                
        return binaries
    
    def test_binary_functionality(self, binary_path: Path, binary_name: str) -> Dict:
        """Test basic functionality of a binary."""
        test_result = {
            'executable': False,
            'version_check': False,
            'help_check': False,
            'error_message': None
        }
        
        try:
            # Test if binary is executable
            if not os.access(binary_path, os.X_OK):
                test_result['error_message'] = "Binary is not executable"
                return test_result
            test_result['executable'] = True
            
            # Test version command
            try:
                result = subprocess.run(
                    [str(binary_path), '--version'],
                    capture_output=True,
                    text=True,
                    timeout=30
                )
                if result.returncode == 0 and 'babachain' in result.stdout.lower():
                    test_result['version_check'] = True
                else:
                    test_result['error_message'] = f"Version check failed: {result.stderr}"
            except subprocess.TimeoutExpired:
                test_result['error_message'] = "Version check timed out"
            except Exception as e:
                test_result['error_message'] = f"Version check error: {str(e)}"
            
            # Test help command (except for GUI)
            if binary_name != 'babachain-qt':
                try:
                    result = subprocess.run(
                        [str(binary_path), '--help'],
                        capture_output=True,
                        text=True,
                        timeout=30
                    )
                    if result.returncode == 0 or 'usage' in result.stdout.lower():
                        test_result['help_check'] = True
                except subprocess.TimeoutExpired:
                    test_result['error_message'] = "Help check timed out"
                except Exception as e:
                    test_result['error_message'] = f"Help check error: {str(e)}"
            else:
                # For GUI, just mark help as passed if version works
                test_result['help_check'] = test_result['version_check']
                
        except Exception as e:
            test_result['error_message'] = f"Unexpected error: {str(e)}"
            
        return test_result
    
    def check_binary_architecture(self, binary_path: Path) -> Dict:
        """Check binary architecture and compatibility."""
        arch_info = {
            'detected_arch': None,
            'expected_arch': None,
            'compatible': False,
            'file_info': None
        }
        
        try:
            # Use file command to get binary info
            result = subprocess.run(
                ['file', str(binary_path)],
                capture_output=True,
                text=True,
                timeout=10
            )
            
            if result.returncode == 0:
                arch_info['file_info'] = result.stdout.strip()
                
                # Determine expected architecture from platform name
                if 'arm64' in self.platform_name.lower() or 'aarch64' in self.platform_name.lower():
                    arch_info['expected_arch'] = 'arm64'
                    arch_info['compatible'] = any(x in result.stdout.lower() 
                                                for x in ['arm64', 'aarch64'])
                elif 'x64' in self.platform_name.lower() or 'x86_64' in self.platform_name.lower():
                    arch_info['expected_arch'] = 'x86_64'
                    arch_info['compatible'] = 'x86_64' in result.stdout.lower() or 'x86-64' in result.stdout.lower()
                else:
                    arch_info['expected_arch'] = 'unknown'
                    arch_info['compatible'] = True  # Assume compatible if unknown
                    
                # Extract detected architecture
                if 'x86_64' in result.stdout.lower() or 'x86-64' in result.stdout.lower():
                    arch_info['detected_arch'] = 'x86_64'
                elif 'arm64' in result.stdout.lower() or 'aarch64' in result.stdout.lower():
                    arch_info['detected_arch'] = 'arm64'
                elif 'i386' in result.stdout.lower():
                    arch_info['detected_arch'] = 'i386'
                    
        except Exception as e:
            arch_info['error'] = str(e)
            
        return arch_info
    
    def generate_checksums(self, binary_path: Path) -> Dict[str, str]:
        """Generate multiple checksums for a binary."""
        checksums = {}
        
        try:
            with open(binary_path, 'rb') as f:
                data = f.read()
                
            # Generate multiple hash types
            checksums['sha256'] = hashlib.sha256(data).hexdigest()
            checksums['sha1'] = hashlib.sha1(data).hexdigest()
            checksums['md5'] = hashlib.md5(data).hexdigest()
            checksums['size'] = len(data)
            
        except Exception as e:
            checksums['error'] = str(e)
            
        return checksums
    
    def test_daemon_basic_operations(self, daemon_path: Path) -> Dict:
        """Test basic daemon operations without starting a full node."""
        test_result = {
            'config_test': False,
            'datadir_test': False,
            'rpc_test': False,
            'error_message': None
        }
        
        try:
            with tempfile.TemporaryDirectory() as temp_dir:
                # Test config file parsing
                config_file = Path(temp_dir) / 'babachain.conf'
                config_file.write_text('testnet=1\nrpcuser=test\nrpcpassword=test\n')
                
                result = subprocess.run(
                    [str(daemon_path), f'-conf={config_file}', '-printtoconsole=0', '-?'],
                    capture_output=True,
                    text=True,
                    timeout=30
                )
                
                if result.returncode == 0:
                    test_result['config_test'] = True
                    
                # Test datadir parameter
                result = subprocess.run(
                    [str(daemon_path), f'-datadir={temp_dir}', '-printtoconsole=0', '-?'],
                    capture_output=True,
                    text=True,
                    timeout=30
                )
                
                if result.returncode == 0:
                    test_result['datadir_test'] = True
                    
                # Test RPC help (without starting daemon)
                result = subprocess.run(
                    [str(daemon_path), '-printtoconsole=0', '-?'],
                    capture_output=True,
                    text=True,
                    timeout=30
                )
                
                if result.returncode == 0 and 'rpc' in result.stdout.lower():
                    test_result['rpc_test'] = True
                    
        except Exception as e:
            test_result['error_message'] = str(e)
            
        return test_result
    
    def run_verification(self) -> Dict:
        """Run complete verification suite."""
        print(f"Starting build verification for {self.platform_name}")
        
        # Find binaries
        binaries = self.find_binaries()
        print(f"Found binaries: {[name for name, path in binaries.items() if path is not None]}")
        
        # Check required binaries
        missing_required = []
        for binary_name, required in self.expected_binaries.items():
            if required and binaries[binary_name] is None:
                missing_required.append(binary_name)
                
        if missing_required:
            self.results['error'] = f"Missing required binaries: {missing_required}"
            return self.results
        
        # Test each binary
        for binary_name, binary_path in binaries.items():
            if binary_path is None:
                continue
                
            print(f"Testing {binary_name}...")
            
            # Basic functionality tests
            func_test = self.test_binary_functionality(binary_path, binary_name)
            self.results['tests'][binary_name] = func_test
            
            # Architecture compatibility
            arch_test = self.check_binary_architecture(binary_path)
            self.results['compatibility'][binary_name] = arch_test
            
            # Generate checksums
            checksums = self.generate_checksums(binary_path)
            self.results['checksums'][binary_name] = checksums
            
            # Special tests for daemon
            if binary_name == 'babachaind' and func_test['executable']:
                daemon_test = self.test_daemon_basic_operations(binary_path)
                self.results['tests'][f'{binary_name}_advanced'] = daemon_test
        
        # Overall success determination
        self.results['success'] = self._determine_overall_success()
        
        return self.results
    
    def _determine_overall_success(self) -> bool:
        """Determine if verification was overall successful."""
        # Check that all required binaries passed basic tests
        for binary_name, required in self.expected_binaries.items():
            if required:
                if binary_name not in self.results['tests']:
                    return False
                test_result = self.results['tests'][binary_name]
                if not (test_result['executable'] and test_result['version_check']):
                    return False
                    
                # Check architecture compatibility
                if binary_name in self.results['compatibility']:
                    if not self.results['compatibility'][binary_name].get('compatible', False):
                        return False
        
        return True
    
    def save_results(self, output_file: str):
        """Save verification results to JSON file."""
        with open(output_file, 'w') as f:
            json.dump(self.results, f, indent=2, default=str)
    
    def print_summary(self):
        """Print a human-readable summary of results."""
        print(f"\n=== Build Verification Summary for {self.platform_name} ===")
        print(f"Overall Success: {'✅ PASS' if self.results['success'] else '❌ FAIL'}")
        
        print("\n📦 Binary Tests:")
        for binary_name, test_result in self.results['tests'].items():
            if '_advanced' in binary_name:
                continue
            status = "✅" if test_result.get('executable') and test_result.get('version_check') else "❌"
            print(f"  {status} {binary_name}")
            if test_result.get('error_message'):
                print(f"    Error: {test_result['error_message']}")
        
        print("\n🏗️ Architecture Compatibility:")
        for binary_name, arch_info in self.results['compatibility'].items():
            status = "✅" if arch_info.get('compatible') else "❌"
            expected = arch_info.get('expected_arch', 'unknown')
            detected = arch_info.get('detected_arch', 'unknown')
            print(f"  {status} {binary_name}: {detected} (expected: {expected})")
        
        print("\n🔐 Checksums Generated:")
        for binary_name, checksums in self.results['checksums'].items():
            if 'sha256' in checksums:
                print(f"  📄 {binary_name}: {checksums['sha256'][:16]}... ({checksums['size']} bytes)")


def main():
    parser = argparse.ArgumentParser(description='BabaChain Build Verification Tool')
    parser.add_argument('binary_dir', help='Directory containing binaries to verify')
    parser.add_argument('platform', help='Platform name (e.g., linux-x64, macos-arm64)')
    parser.add_argument('--output', '-o', help='Output JSON file for results')
    parser.add_argument('--quiet', '-q', action='store_true', help='Quiet mode')
    
    args = parser.parse_args()
    
    if not os.path.exists(args.binary_dir):
        print(f"Error: Binary directory '{args.binary_dir}' does not exist")
        sys.exit(1)
    
    verifier = BuildVerifier(args.binary_dir, args.platform)
    results = verifier.run_verification()
    
    if not args.quiet:
        verifier.print_summary()
    
    if args.output:
        verifier.save_results(args.output)
        if not args.quiet:
            print(f"\nResults saved to: {args.output}")
    
    # Exit with error code if verification failed
    sys.exit(0 if results['success'] else 1)


if __name__ == '__main__':
    main()