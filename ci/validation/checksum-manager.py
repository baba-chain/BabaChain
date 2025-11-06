#!/usr/bin/env python3
"""
BabaChain Checksum Manager
Generates and verifies checksums for build artifacts
"""

import os
import sys
import json
import hashlib
import hmac
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass, asdict
from datetime import datetime

@dataclass
class ChecksumEntry:
    filename: str
    size: int
    sha256: str
    sha512: str
    md5: str
    timestamp: str
    platform: str
    version: str

@dataclass
class ChecksumManifest:
    version: str
    timestamp: str
    build_id: str
    git_commit: str
    git_branch: str
    entries: List[ChecksumEntry]
    signature: Optional[str] = None

class ChecksumManager:
    def __init__(self, signing_key: Optional[str] = None):
        self.signing_key = signing_key
        self.supported_algorithms = ['md5', 'sha256', 'sha512']
    
    def calculate_checksums(self, file_path: Path) -> Dict[str, str]:
        """Calculate multiple checksums for a file"""
        checksums = {}
        hash_objects = {
            'md5': hashlib.md5(),
            'sha256': hashlib.sha256(),
            'sha512': hashlib.sha512()
        }
        
        with open(file_path, 'rb') as f:
            while chunk := f.read(8192):
                for hash_obj in hash_objects.values():
                    hash_obj.update(chunk)
        
        for algorithm, hash_obj in hash_objects.items():
            checksums[algorithm] = hash_obj.hexdigest()
        
        return checksums
    
    def generate_checksum_entry(self, file_path: Path, platform: str = "unknown", 
                               version: str = "unknown") -> ChecksumEntry:
        """Generate checksum entry for a single file"""
        if not file_path.exists():
            raise FileNotFoundError(f"File not found: {file_path}")
        
        checksums = self.calculate_checksums(file_path)
        file_size = file_path.stat().st_size
        
        return ChecksumEntry(
            filename=file_path.name,
            size=file_size,
            sha256=checksums['sha256'],
            sha512=checksums['sha512'],
            md5=checksums['md5'],
            timestamp=datetime.now().isoformat(),
            platform=platform,
            version=version
        )
    
    def generate_manifest(self, artifact_paths: List[Path], output_path: Path,
                         version: str = "unknown", build_id: str = "unknown") -> ChecksumManifest:
        """Generate checksum manifest for multiple artifacts"""
        entries = []
        
        for artifact_path in artifact_paths:
            if not artifact_path.exists():
                print(f"Warning: Artifact not found: {artifact_path}")
                continue
            
            # Detect platform from filename
            platform = self._detect_platform_from_filename(artifact_path.name)
            
            entry = self.generate_checksum_entry(artifact_path, platform, version)
            entries.append(entry)
        
        # Get git information
        git_commit = self._get_git_commit()
        git_branch = self._get_git_branch()
        
        manifest = ChecksumManifest(
            version="1.0",
            timestamp=datetime.now().isoformat(),
            build_id=build_id,
            git_commit=git_commit,
            git_branch=git_branch,
            entries=entries
        )
        
        # Sign manifest if signing key is provided
        if self.signing_key:
            manifest.signature = self._sign_manifest(manifest)
        
        # Save manifest
        with open(output_path, 'w') as f:
            json.dump(asdict(manifest), f, indent=2)
        
        return manifest
    
    def verify_checksums(self, manifest_path: Path, artifacts_dir: Path) -> Tuple[bool, List[str]]:
        """Verify checksums against manifest"""
        if not manifest_path.exists():
            return False, [f"Manifest not found: {manifest_path}"]
        
        with open(manifest_path, 'r') as f:
            manifest_data = json.load(f)
        
        manifest = ChecksumManifest(**manifest_data)
        errors = []
        
        # Verify signature if present
        if manifest.signature and self.signing_key:
            if not self._verify_signature(manifest):
                errors.append("Manifest signature verification failed")
        
        # Verify each entry
        for entry in manifest.entries:
            artifact_path = artifacts_dir / entry.filename
            
            if not artifact_path.exists():
                errors.append(f"Artifact not found: {entry.filename}")
                continue
            
            # Verify file size
            actual_size = artifact_path.stat().st_size
            if actual_size != entry.size:
                errors.append(f"Size mismatch for {entry.filename}: expected {entry.size}, got {actual_size}")
                continue
            
            # Verify checksums
            actual_checksums = self.calculate_checksums(artifact_path)
            
            if actual_checksums['sha256'] != entry.sha256:
                errors.append(f"SHA256 mismatch for {entry.filename}")
            
            if actual_checksums['sha512'] != entry.sha512:
                errors.append(f"SHA512 mismatch for {entry.filename}")
            
            if actual_checksums['md5'] != entry.md5:
                errors.append(f"MD5 mismatch for {entry.filename}")
        
        return len(errors) == 0, errors
    
    def generate_checksums_file(self, artifact_paths: List[Path], output_path: Path,
                               format_type: str = "sha256sum"):
        """Generate checksums file in various formats"""
        with open(output_path, 'w') as f:
            for artifact_path in artifact_paths:
                if not artifact_path.exists():
                    continue
                
                checksums = self.calculate_checksums(artifact_path)
                
                if format_type == "sha256sum":
                    f.write(f"{checksums['sha256']}  {artifact_path.name}\n")
                elif format_type == "sha512sum":
                    f.write(f"{checksums['sha512']}  {artifact_path.name}\n")
                elif format_type == "md5sum":
                    f.write(f"{checksums['md5']}  {artifact_path.name}\n")
                elif format_type == "all":
                    f.write(f"# {artifact_path.name}\n")
                    f.write(f"MD5:    {checksums['md5']}\n")
                    f.write(f"SHA256: {checksums['sha256']}\n")
                    f.write(f"SHA512: {checksums['sha512']}\n")
                    f.write(f"Size:   {artifact_path.stat().st_size} bytes\n\n")
    
    def verify_checksums_file(self, checksums_file: Path, artifacts_dir: Path,
                             format_type: str = "sha256sum") -> Tuple[bool, List[str]]:
        """Verify checksums from checksums file"""
        if not checksums_file.exists():
            return False, [f"Checksums file not found: {checksums_file}"]
        
        errors = []
        
        with open(checksums_file, 'r') as f:
            for line_num, line in enumerate(f, 1):
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                
                try:
                    if format_type in ["sha256sum", "sha512sum", "md5sum"]:
                        parts = line.split(None, 1)
                        if len(parts) != 2:
                            errors.append(f"Invalid format at line {line_num}")
                            continue
                        
                        expected_hash, filename = parts
                        artifact_path = artifacts_dir / filename
                        
                        if not artifact_path.exists():
                            errors.append(f"Artifact not found: {filename}")
                            continue
                        
                        actual_checksums = self.calculate_checksums(artifact_path)
                        
                        if format_type == "sha256sum":
                            actual_hash = actual_checksums['sha256']
                        elif format_type == "sha512sum":
                            actual_hash = actual_checksums['sha512']
                        elif format_type == "md5sum":
                            actual_hash = actual_checksums['md5']
                        
                        if actual_hash != expected_hash:
                            errors.append(f"Checksum mismatch for {filename}")
                
                except Exception as e:
                    errors.append(f"Error processing line {line_num}: {e}")
        
        return len(errors) == 0, errors
    
    def _detect_platform_from_filename(self, filename: str) -> str:
        """Detect platform from filename"""
        filename_lower = filename.lower()
        
        if 'linux-x64' in filename_lower or 'linux-amd64' in filename_lower:
            return 'linux-x64'
        elif 'linux-arm64' in filename_lower or 'linux-aarch64' in filename_lower:
            return 'linux-arm64'
        elif 'macos-x64' in filename_lower or 'darwin-x64' in filename_lower:
            return 'macos-x64'
        elif 'macos-arm64' in filename_lower or 'darwin-arm64' in filename_lower:
            return 'macos-arm64'
        elif 'windows-x64' in filename_lower or 'win64' in filename_lower:
            return 'windows-x64'
        else:
            return 'unknown'
    
    def _get_git_commit(self) -> str:
        """Get current git commit"""
        import subprocess
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
        return os.getenv('GITHUB_SHA', 'unknown')
    
    def _get_git_branch(self) -> str:
        """Get current git branch"""
        import subprocess
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
        return os.getenv('GITHUB_REF_NAME', 'unknown')
    
    def _sign_manifest(self, manifest: ChecksumManifest) -> str:
        """Sign manifest with HMAC"""
        # Create a copy without signature for signing
        manifest_copy = ChecksumManifest(
            version=manifest.version,
            timestamp=manifest.timestamp,
            build_id=manifest.build_id,
            git_commit=manifest.git_commit,
            git_branch=manifest.git_branch,
            entries=manifest.entries
        )
        
        manifest_json = json.dumps(asdict(manifest_copy), sort_keys=True)
        signature = hmac.new(
            self.signing_key.encode(),
            manifest_json.encode(),
            hashlib.sha256
        ).hexdigest()
        
        return signature
    
    def _verify_signature(self, manifest: ChecksumManifest) -> bool:
        """Verify manifest signature"""
        if not manifest.signature or not self.signing_key:
            return False
        
        expected_signature = self._sign_manifest(manifest)
        return hmac.compare_digest(manifest.signature, expected_signature)
    
    def generate_release_checksums(self, release_dir: Path, version: str):
        """Generate comprehensive checksums for a release"""
        release_dir = Path(release_dir)
        if not release_dir.exists():
            raise FileNotFoundError(f"Release directory not found: {release_dir}")
        
        # Find all artifacts
        artifact_patterns = [
            "*.tar.gz", "*.zip", "*.dmg", "*.exe", "*.deb", "*.rpm"
        ]
        
        artifacts = []
        for pattern in artifact_patterns:
            artifacts.extend(release_dir.glob(pattern))
        
        if not artifacts:
            print("Warning: No artifacts found in release directory")
            return
        
        print(f"Found {len(artifacts)} artifacts to process")
        
        # Generate manifest
        manifest_path = release_dir / "checksums.json"
        build_id = os.getenv('GITHUB_RUN_ID', f"local-{datetime.now().strftime('%Y%m%d-%H%M%S')}")
        
        manifest = self.generate_manifest(artifacts, manifest_path, version, build_id)
        print(f"Generated manifest: {manifest_path}")
        
        # Generate traditional checksum files
        for format_type in ["sha256sum", "sha512sum", "md5sum"]:
            checksum_file = release_dir / f"checksums.{format_type}"
            self.generate_checksums_file(artifacts, checksum_file, format_type)
            print(f"Generated {format_type} file: {checksum_file}")
        
        # Generate comprehensive checksums file
        all_checksums_file = release_dir / "CHECKSUMS.txt"
        self.generate_checksums_file(artifacts, all_checksums_file, "all")
        print(f"Generated comprehensive checksums: {all_checksums_file}")
        
        # Generate verification script
        self._generate_verification_script(release_dir)
        print(f"Generated verification script: {release_dir / 'verify-checksums.sh'}")
    
    def _generate_verification_script(self, release_dir: Path):
        """Generate shell script for checksum verification"""
        script_path = release_dir / "verify-checksums.sh"
        
        script_content = '''#!/bin/bash
# BabaChain Release Checksum Verification Script
# This script verifies the integrity of downloaded BabaChain release files

set -euo pipefail

RED='\\033[0;31m'
GREEN='\\033[0;32m'
YELLOW='\\033[1;33m'
NC='\\033[0m' # No Color

echo "🔍 BabaChain Release Checksum Verification"
echo "=========================================="

# Check if checksum files exist
if [[ ! -f "checksums.sha256sum" ]]; then
    echo -e "${RED}❌ checksums.sha256sum not found${NC}"
    exit 1
fi

# Verify SHA256 checksums
echo "Verifying SHA256 checksums..."
if sha256sum -c checksums.sha256sum; then
    echo -e "${GREEN}✅ All SHA256 checksums verified successfully${NC}"
else
    echo -e "${RED}❌ SHA256 checksum verification failed${NC}"
    exit 1
fi

# Verify SHA512 checksums if available
if [[ -f "checksums.sha512sum" ]]; then
    echo "Verifying SHA512 checksums..."
    if sha512sum -c checksums.sha512sum; then
        echo -e "${GREEN}✅ All SHA512 checksums verified successfully${NC}"
    else
        echo -e "${RED}❌ SHA512 checksum verification failed${NC}"
        exit 1
    fi
fi

# Verify MD5 checksums if available
if [[ -f "checksums.md5sum" ]]; then
    echo "Verifying MD5 checksums..."
    if md5sum -c checksums.md5sum; then
        echo -e "${GREEN}✅ All MD5 checksums verified successfully${NC}"
    else
        echo -e "${RED}❌ MD5 checksum verification failed${NC}"
        exit 1
    fi
fi

echo -e "${GREEN}🎉 All checksums verified successfully!${NC}"
echo "Your BabaChain release files are authentic and have not been tampered with."
'''
        
        with open(script_path, 'w') as f:
            f.write(script_content)
        
        # Make script executable
        os.chmod(script_path, 0o755)

def main():
    if len(sys.argv) < 2:
        print("Usage: checksum-manager.py <command> [options]")
        print("Commands:")
        print("  generate <file1> [file2] ... - Generate checksums for files")
        print("  verify <checksums_file> <artifacts_dir> - Verify checksums")
        print("  manifest <artifacts_dir> <version> - Generate manifest")
        print("  release <release_dir> <version> - Generate release checksums")
        sys.exit(1)
    
    command = sys.argv[1]
    signing_key = os.getenv('CHECKSUM_SIGNING_KEY')
    
    manager = ChecksumManager(signing_key)
    
    if command == "generate":
        if len(sys.argv) < 3:
            print("Usage: checksum-manager.py generate <file1> [file2] ...")
            sys.exit(1)
        
        for file_path in sys.argv[2:]:
            path = Path(file_path)
            if path.exists():
                checksums = manager.calculate_checksums(path)
                print(f"File: {path.name}")
                print(f"  MD5:    {checksums['md5']}")
                print(f"  SHA256: {checksums['sha256']}")
                print(f"  SHA512: {checksums['sha512']}")
                print(f"  Size:   {path.stat().st_size} bytes")
                print()
            else:
                print(f"File not found: {file_path}")
    
    elif command == "verify":
        if len(sys.argv) < 4:
            print("Usage: checksum-manager.py verify <checksums_file> <artifacts_dir>")
            sys.exit(1)
        
        checksums_file = Path(sys.argv[2])
        artifacts_dir = Path(sys.argv[3])
        
        success, errors = manager.verify_checksums_file(checksums_file, artifacts_dir)
        
        if success:
            print("✅ All checksums verified successfully")
        else:
            print("❌ Checksum verification failed:")
            for error in errors:
                print(f"  - {error}")
            sys.exit(1)
    
    elif command == "manifest":
        if len(sys.argv) < 4:
            print("Usage: checksum-manager.py manifest <artifacts_dir> <version>")
            sys.exit(1)
        
        artifacts_dir = Path(sys.argv[2])
        version = sys.argv[3]
        
        artifacts = list(artifacts_dir.glob("*"))
        artifacts = [p for p in artifacts if p.is_file()]
        
        manifest_path = artifacts_dir / "checksums.json"
        build_id = os.getenv('GITHUB_RUN_ID', 'local')
        
        manifest = manager.generate_manifest(artifacts, manifest_path, version, build_id)
        print(f"Generated manifest with {len(manifest.entries)} entries")
    
    elif command == "release":
        if len(sys.argv) < 4:
            print("Usage: checksum-manager.py release <release_dir> <version>")
            sys.exit(1)
        
        release_dir = Path(sys.argv[2])
        version = sys.argv[3]
        
        manager.generate_release_checksums(release_dir, version)
        print("Release checksums generated successfully")
    
    else:
        print(f"Unknown command: {command}")
        sys.exit(1)

if __name__ == "__main__":
    main()