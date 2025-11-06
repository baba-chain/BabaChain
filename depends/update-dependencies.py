#!/usr/bin/env python3
"""
BabaChain Dependency Update Automation Script

This script automates the process of checking for dependency updates,
validating compatibility, and updating package definitions.
"""

import os
import sys
import json
import hashlib
import urllib.request
import urllib.error
import subprocess
import argparse
import logging
from datetime import datetime
from pathlib import Path
import re

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('update-logs/dependency-updates.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)

class DependencyUpdater:
    def __init__(self, basedir=None):
        self.basedir = Path(basedir) if basedir else Path(__file__).parent
        self.packages_dir = self.basedir / 'packages'
        self.update_logs_dir = self.basedir / 'update-logs'
        self.update_logs_dir.mkdir(exist_ok=True)
        
        # Create update status file
        self.status_file = self.update_logs_dir / 'update-status.json'
        self.load_status()
    
    def load_status(self):
        """Load update status from file"""
        try:
            if self.status_file.exists():
                with open(self.status_file, 'r') as f:
                    self.status = json.load(f)
            else:
                self.status = {
                    'last_check': None,
                    'packages': {},
                    'failed_updates': [],
                    'successful_updates': []
                }
        except Exception as e:
            logger.error(f"Failed to load status: {e}")
            self.status = {'last_check': None, 'packages': {}, 'failed_updates': [], 'successful_updates': []}
    
    def save_status(self):
        """Save update status to file"""
        try:
            self.status['last_check'] = datetime.now().isoformat()
            with open(self.status_file, 'w') as f:
                json.dump(self.status, f, indent=2)
        except Exception as e:
            logger.error(f"Failed to save status: {e}")
    
    def get_package_files(self):
        """Get all package definition files"""
        package_files = []
        for file_path in self.packages_dir.glob('*.mk'):
            if file_path.name != 'packages.mk':
                package_files.append(file_path)
        return package_files
    
    def parse_package_file(self, file_path):
        """Parse a package definition file"""
        package_info = {
            'name': file_path.stem,
            'file_path': file_path,
            'version': None,
            'url': None,
            'sha256': None,
            'download_path': None
        }
        
        try:
            with open(file_path, 'r') as f:
                content = f.read()
            
            # Extract version
            version_match = re.search(r'(\w+)_version\s*[:=]\s*([^\s\n]+)', content)
            if version_match:
                package_info['version'] = version_match.group(2)
            
            # Extract URL
            url_match = re.search(r'(\w+)_url\s*[:=]\s*([^\s\n]+)', content)
            if url_match:
                package_info['url'] = url_match.group(2)
            
            # Extract SHA256
            sha256_match = re.search(r'(\w+)_sha256\s*[:=]\s*([a-fA-F0-9]{64})', content)
            if sha256_match:
                package_info['sha256'] = sha256_match.group(2)
            
            # Extract download path
            download_match = re.search(r'(\w+)_download_path\s*[:=]\s*([^\s\n]+)', content)
            if download_match:
                package_info['download_path'] = download_match.group(2)
                
        except Exception as e:
            logger.error(f"Failed to parse {file_path}: {e}")
        
        return package_info
    
    def check_url_availability(self, url, timeout=10):
        """Check if a URL is accessible"""
        try:
            req = urllib.request.Request(url, method='HEAD')
            with urllib.request.urlopen(req, timeout=timeout) as response:
                return response.status == 200
        except Exception as e:
            logger.debug(f"URL check failed for {url}: {e}")
            return False
    
    def calculate_sha256(self, file_path):
        """Calculate SHA256 hash of a file"""
        sha256_hash = hashlib.sha256()
        try:
            with open(file_path, "rb") as f:
                for chunk in iter(lambda: f.read(4096), b""):
                    sha256_hash.update(chunk)
            return sha256_hash.hexdigest()
        except Exception as e:
            logger.error(f"Failed to calculate SHA256 for {file_path}: {e}")
            return None
    
    def download_and_verify(self, url, expected_sha256=None):
        """Download a file and verify its checksum"""
        try:
            # Create temporary download directory
            temp_dir = self.basedir / 'temp_downloads'
            temp_dir.mkdir(exist_ok=True)
            
            # Extract filename from URL
            filename = url.split('/')[-1]
            temp_file = temp_dir / filename
            
            # Download file
            logger.info(f"Downloading {url}")
            urllib.request.urlretrieve(url, temp_file)
            
            # Verify checksum if provided
            if expected_sha256:
                actual_sha256 = self.calculate_sha256(temp_file)
                if actual_sha256 != expected_sha256:
                    logger.error(f"Checksum mismatch for {filename}")
                    logger.error(f"Expected: {expected_sha256}")
                    logger.error(f"Actual: {actual_sha256}")
                    return False, actual_sha256
                else:
                    logger.info(f"Checksum verified for {filename}")
            
            # Clean up
            temp_file.unlink()
            return True, actual_sha256 if expected_sha256 else None
            
        except Exception as e:
            logger.error(f"Download failed for {url}: {e}")
            return False, None
    
    def check_package_updates(self):
        """Check all packages for available updates"""
        logger.info("Checking for package updates...")
        
        package_files = self.get_package_files()
        update_report = {
            'timestamp': datetime.now().isoformat(),
            'total_packages': len(package_files),
            'packages_checked': 0,
            'packages_with_issues': [],
            'url_check_results': {},
            'recommendations': []
        }
        
        for package_file in package_files:
            try:
                package_info = self.parse_package_file(package_file)
                package_name = package_info['name']
                
                logger.info(f"Checking package: {package_name}")
                update_report['packages_checked'] += 1
                
                # Check URL availability
                if package_info['url']:
                    url_available = self.check_url_availability(package_info['url'])
                    update_report['url_check_results'][package_name] = {
                        'url': package_info['url'],
                        'available': url_available,
                        'version': package_info['version']
                    }
                    
                    if not url_available:
                        logger.warning(f"URL not accessible for {package_name}: {package_info['url']}")
                        update_report['packages_with_issues'].append({
                            'package': package_name,
                            'issue': 'URL not accessible',
                            'url': package_info['url']
                        })
                
                # Store package info in status
                self.status['packages'][package_name] = {
                    'version': package_info['version'],
                    'url': package_info['url'],
                    'sha256': package_info['sha256'],
                    'last_checked': datetime.now().isoformat(),
                    'url_accessible': url_available if package_info['url'] else None
                }
                
            except Exception as e:
                logger.error(f"Failed to check package {package_file}: {e}")
                update_report['packages_with_issues'].append({
                    'package': package_file.name,
                    'issue': f'Check failed: {str(e)}'
                })
        
        # Generate recommendations
        if update_report['packages_with_issues']:
            update_report['recommendations'].append(
                "Review packages with issues and update URLs or versions as needed"
            )
        
        # Save report
        report_file = self.update_logs_dir / f'update-check-{datetime.now().strftime("%Y%m%d-%H%M%S")}.json'
        with open(report_file, 'w') as f:
            json.dump(update_report, f, indent=2)
        
        logger.info(f"Update check completed. Report saved to {report_file}")
        return update_report
    
    def verify_dependencies(self):
        """Verify integrity of all dependencies"""
        logger.info("Verifying dependency integrity...")
        
        verification_report = {
            'timestamp': datetime.now().isoformat(),
            'verified_packages': [],
            'failed_verifications': [],
            'missing_checksums': []
        }
        
        package_files = self.get_package_files()
        
        for package_file in package_files:
            try:
                package_info = self.parse_package_file(package_file)
                package_name = package_info['name']
                
                logger.info(f"Verifying package: {package_name}")
                
                if not package_info['sha256']:
                    logger.warning(f"No SHA256 checksum found for {package_name}")
                    verification_report['missing_checksums'].append(package_name)
                    continue
                
                if not package_info['url']:
                    logger.warning(f"No URL found for {package_name}")
                    continue
                
                # Verify download and checksum
                success, actual_sha256 = self.download_and_verify(
                    package_info['url'], 
                    package_info['sha256']
                )
                
                if success:
                    verification_report['verified_packages'].append({
                        'package': package_name,
                        'version': package_info['version'],
                        'sha256': package_info['sha256']
                    })
                    logger.info(f"✅ Verified {package_name}")
                else:
                    verification_report['failed_verifications'].append({
                        'package': package_name,
                        'version': package_info['version'],
                        'expected_sha256': package_info['sha256'],
                        'actual_sha256': actual_sha256
                    })
                    logger.error(f"❌ Verification failed for {package_name}")
                
            except Exception as e:
                logger.error(f"Verification error for {package_file}: {e}")
                verification_report['failed_verifications'].append({
                    'package': package_file.name,
                    'error': str(e)
                })
        
        # Save verification report
        report_file = self.update_logs_dir / f'verification-{datetime.now().strftime("%Y%m%d-%H%M%S")}.json'
        with open(report_file, 'w') as f:
            json.dump(verification_report, f, indent=2)
        
        logger.info(f"Verification completed. Report saved to {report_file}")
        return verification_report
    
    def generate_status_report(self):
        """Generate a comprehensive status report"""
        logger.info("Generating status report...")
        
        status_report = {
            'timestamp': datetime.now().isoformat(),
            'last_check': self.status.get('last_check'),
            'total_packages': len(self.status.get('packages', {})),
            'packages_with_accessible_urls': 0,
            'packages_with_inaccessible_urls': 0,
            'packages_without_urls': 0,
            'recent_successful_updates': len(self.status.get('successful_updates', [])),
            'recent_failed_updates': len(self.status.get('failed_updates', [])),
            'package_summary': []
        }
        
        for package_name, package_data in self.status.get('packages', {}).items():
            url_status = package_data.get('url_accessible')
            if url_status is True:
                status_report['packages_with_accessible_urls'] += 1
            elif url_status is False:
                status_report['packages_with_inaccessible_urls'] += 1
            else:
                status_report['packages_without_urls'] += 1
            
            status_report['package_summary'].append({
                'name': package_name,
                'version': package_data.get('version'),
                'url_accessible': url_status,
                'last_checked': package_data.get('last_checked')
            })
        
        # Save status report
        report_file = self.update_logs_dir / 'latest-status-report.json'
        with open(report_file, 'w') as f:
            json.dump(status_report, f, indent=2)
        
        # Create human-readable summary
        summary_file = self.update_logs_dir / 'status-summary.md'
        with open(summary_file, 'w') as f:
            f.write(f"# Dependency Status Report\n\n")
            f.write(f"**Generated:** {status_report['timestamp']}\n")
            f.write(f"**Last Check:** {status_report['last_check'] or 'Never'}\n\n")
            f.write(f"## Summary\n\n")
            f.write(f"- Total Packages: {status_report['total_packages']}\n")
            f.write(f"- ✅ Accessible URLs: {status_report['packages_with_accessible_urls']}\n")
            f.write(f"- ❌ Inaccessible URLs: {status_report['packages_with_inaccessible_urls']}\n")
            f.write(f"- ⚠️ No URLs: {status_report['packages_without_urls']}\n")
            f.write(f"- Recent Successful Updates: {status_report['recent_successful_updates']}\n")
            f.write(f"- Recent Failed Updates: {status_report['recent_failed_updates']}\n\n")
            
            if status_report['packages_with_inaccessible_urls'] > 0:
                f.write(f"## ⚠️ Packages with Inaccessible URLs\n\n")
                for package in status_report['package_summary']:
                    if package['url_accessible'] is False:
                        f.write(f"- **{package['name']}** (v{package['version']})\n")
        
        logger.info(f"Status report generated: {report_file}")
        return status_report

def main():
    parser = argparse.ArgumentParser(description='BabaChain Dependency Update Automation')
    parser.add_argument('--action', choices=['check', 'verify', 'status', 'all'], 
                       default='all', help='Action to perform')
    parser.add_argument('--basedir', help='Base directory (default: script directory)')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose logging')
    
    args = parser.parse_args()
    
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    
    updater = DependencyUpdater(args.basedir)
    
    try:
        if args.action in ['check', 'all']:
            updater.check_package_updates()
        
        if args.action in ['verify', 'all']:
            updater.verify_dependencies()
        
        if args.action in ['status', 'all']:
            updater.generate_status_report()
        
        updater.save_status()
        logger.info("Dependency update automation completed successfully")
        
    except Exception as e:
        logger.error(f"Dependency update automation failed: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()