#!/usr/bin/env python3
# Copyright (c) 2024 The BabaChain Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Generate and verify checksums for BabaChain release artifacts.
"""

import argparse
import hashlib
import json
import os
import sys
from pathlib import Path
from typing import Dict, List


def calculate_file_hashes(file_path: Path) -> Dict[str, str]:
    """Calculate multiple hash types for a file."""
    hashes = {}
    
    try:
        with open(file_path, 'rb') as f:
            data = f.read()
            
        hashes['sha256'] = hashlib.sha256(data).hexdigest()
        hashes['sha1'] = hashlib.sha1(data).hexdigest()
        hashes['md5'] = hashlib.md5(data).hexdigest()
        hashes['size'] = len(data)
        hashes['filename'] = file_path.name
        
    except Exception as e:
        hashes['error'] = str(e)
        
    return hashes


def generate_checksums_for_directory(directory: Path, patterns: List[str] = None) -> Dict:
    """Generate checksums for all matching files in a directory."""
    if patterns is None:
        patterns = ['*.tar.gz', '*.zip', '*.exe', '*.dmg', '*.deb', '*.rpm']
    
    checksums = {
        'directory': str(directory),
        'files': {},
        'summary': {
            'total_files': 0,
            'total_size': 0,
            'patterns': patterns
        }
    }
    
    for pattern in patterns:
        for file_path in directory.glob(pattern):
            if file_path.is_file():
                file_hashes = calculate_file_hashes(file_path)
                checksums['files'][str(file_path.relative_to(directory))] = file_hashes
                
                if 'size' in file_hashes:
                    checksums['summary']['total_files'] += 1
                    checksums['summary']['total_size'] += file_hashes['size']
    
    return checksums


def generate_sha256sums_file(checksums: Dict, output_file: Path):
    """Generate a SHA256SUMS file in standard format."""
    with open(output_file, 'w') as f:
        for filename, file_hashes in checksums['files'].items():
            if 'sha256' in file_hashes:
                f.write(f"{file_hashes['sha256']}  {filename}\n")


def generate_checksums_json(checksums: Dict, output_file: Path):
    """Generate a comprehensive JSON checksums file."""
    with open(output_file, 'w') as f:
        json.dump(checksums, f, indent=2)


def verify_checksums(checksums_file: Path, directory: Path) -> Dict:
    """Verify checksums against actual files."""
    verification_results = {
        'verified': {},
        'missing': [],
        'mismatched': [],
        'errors': [],
        'summary': {
            'total_files': 0,
            'verified_files': 0,
            'failed_files': 0
        }
    }
    
    try:
        with open(checksums_file, 'r') as f:
            if checksums_file.suffix == '.json':
                checksums_data = json.load(f)
                files_to_verify = checksums_data.get('files', {})
            else:
                # Assume SHA256SUMS format
                files_to_verify = {}
                for line in f:
                    line = line.strip()
                    if line and not line.startswith('#'):
                        parts = line.split('  ', 1)
                        if len(parts) == 2:
                            files_to_verify[parts[1]] = {'sha256': parts[0]}
        
        for filename, expected_hashes in files_to_verify.items():
            verification_results['summary']['total_files'] += 1
            file_path = directory / filename
            
            if not file_path.exists():
                verification_results['missing'].append(filename)
                verification_results['summary']['failed_files'] += 1
                continue
            
            try:
                actual_hashes = calculate_file_hashes(file_path)
                
                # Verify SHA256 (primary hash)
                if 'sha256' in expected_hashes and 'sha256' in actual_hashes:
                    if expected_hashes['sha256'] == actual_hashes['sha256']:
                        verification_results['verified'][filename] = {
                            'status': 'verified',
                            'sha256': actual_hashes['sha256']
                        }
                        verification_results['summary']['verified_files'] += 1
                    else:
                        verification_results['mismatched'].append({
                            'filename': filename,
                            'expected': expected_hashes['sha256'],
                            'actual': actual_hashes['sha256']
                        })
                        verification_results['summary']['failed_files'] += 1
                else:
                    verification_results['errors'].append(f"Missing SHA256 hash for {filename}")
                    verification_results['summary']['failed_files'] += 1
                    
            except Exception as e:
                verification_results['errors'].append(f"Error verifying {filename}: {str(e)}")
                verification_results['summary']['failed_files'] += 1
                
    except Exception as e:
        verification_results['errors'].append(f"Error reading checksums file: {str(e)}")
    
    return verification_results


def print_verification_summary(results: Dict):
    """Print a human-readable verification summary."""
    summary = results['summary']
    print(f"\n=== Checksum Verification Summary ===")
    print(f"Total files: {summary['total_files']}")
    print(f"Verified: {summary['verified_files']} ✅")
    print(f"Failed: {summary['failed_files']} ❌")
    
    if results['missing']:
        print(f"\nMissing files ({len(results['missing'])}):")
        for filename in results['missing']:
            print(f"  ❌ {filename}")
    
    if results['mismatched']:
        print(f"\nChecksum mismatches ({len(results['mismatched'])}):")
        for mismatch in results['mismatched']:
            print(f"  ❌ {mismatch['filename']}")
            print(f"     Expected: {mismatch['expected']}")
            print(f"     Actual:   {mismatch['actual']}")
    
    if results['errors']:
        print(f"\nErrors ({len(results['errors'])}):")
        for error in results['errors']:
            print(f"  ⚠️  {error}")


def main():
    parser = argparse.ArgumentParser(description='BabaChain Checksum Generator and Verifier')
    subparsers = parser.add_subparsers(dest='command', help='Available commands')
    
    # Generate command
    gen_parser = subparsers.add_parser('generate', help='Generate checksums')
    gen_parser.add_argument('directory', help='Directory containing files to checksum')
    gen_parser.add_argument('--output', '-o', help='Output file (default: SHA256SUMS)')
    gen_parser.add_argument('--json', action='store_true', help='Output in JSON format')
    gen_parser.add_argument('--patterns', nargs='+', 
                           default=['*.tar.gz', '*.zip', '*.exe', '*.dmg'],
                           help='File patterns to include')
    
    # Verify command
    verify_parser = subparsers.add_parser('verify', help='Verify checksums')
    verify_parser.add_argument('checksums_file', help='Checksums file to verify against')
    verify_parser.add_argument('directory', help='Directory containing files to verify')
    verify_parser.add_argument('--output', '-o', help='Output verification results to JSON file')
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        sys.exit(1)
    
    if args.command == 'generate':
        directory = Path(args.directory)
        if not directory.exists():
            print(f"Error: Directory '{directory}' does not exist")
            sys.exit(1)
        
        print(f"Generating checksums for files in {directory}")
        checksums = generate_checksums_for_directory(directory, args.patterns)
        
        if args.output:
            output_file = Path(args.output)
        else:
            output_file = directory / ('checksums.json' if args.json else 'SHA256SUMS')
        
        if args.json:
            generate_checksums_json(checksums, output_file)
        else:
            generate_sha256sums_file(checksums, output_file)
        
        print(f"Generated checksums for {checksums['summary']['total_files']} files")
        print(f"Total size: {checksums['summary']['total_size']:,} bytes")
        print(f"Checksums saved to: {output_file}")
        
        # Print checksums to stdout for CI
        print(f"\n=== SHA256 Checksums ===")
        for filename, file_hashes in checksums['files'].items():
            if 'sha256' in file_hashes:
                print(f"{file_hashes['sha256']}  {filename}")
    
    elif args.command == 'verify':
        checksums_file = Path(args.checksums_file)
        directory = Path(args.directory)
        
        if not checksums_file.exists():
            print(f"Error: Checksums file '{checksums_file}' does not exist")
            sys.exit(1)
        
        if not directory.exists():
            print(f"Error: Directory '{directory}' does not exist")
            sys.exit(1)
        
        print(f"Verifying checksums from {checksums_file}")
        results = verify_checksums(checksums_file, directory)
        
        print_verification_summary(results)
        
        if args.output:
            with open(args.output, 'w') as f:
                json.dump(results, f, indent=2)
            print(f"\nVerification results saved to: {args.output}")
        
        # Exit with error if verification failed
        if results['summary']['failed_files'] > 0:
            sys.exit(1)


if __name__ == '__main__':
    main()