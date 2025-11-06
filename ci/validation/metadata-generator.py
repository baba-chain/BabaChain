#!/usr/bin/env python3
"""
BabaChain Artifact Metadata Generator
Generates comprehensive metadata for build artifacts
"""

import os
import sys
import json
import subprocess
import platform as platform_module
from pathlib import Path
from typing import Dict, List, Optional, Any
from dataclasses import dataclass, asdict
from datetime import datetime

@dataclass
class BuildEnvironment:
    os_name: str
    os_version: str
    architecture: str
    compiler: str
    compiler_version: str
    build_tools: Dict[str, str]
    environment_variables: Dict[str, str]

@dataclass
class GitInformation:
    commit_hash: str
    branch: str
    tag: Optional[str]
    commit_message: str
    commit_author: str
    commit_date: str
    repository_url: str
    is_dirty: bool

@dataclass
class DependencyInfo:
    name: str
    version: str
    source: str
    checksum: Optional[str] = None

@dataclass
class BinaryInfo:
    name: str
    size: int
    architecture: str
    format: str
    stripped: bool
    static_linked: bool
    dependencies: List[str]
    entry_point: Optional[str] = None

@dataclass
class ArtifactMetadata:
    # Basic information
    name: str
    version: str
    build_id: str
    timestamp: str
    
    # Platform information
    target_platform: str
    target_architecture: str
    
    # Build information
    build_environment: BuildEnvironment
    git_info: GitInformation
    dependencies: List[DependencyInfo]
    
    # Binary information
    binaries: List[BinaryInfo]
    
    # Checksums
    checksums: Dict[str, str]
    
    # Additional metadata
    build_flags: List[str]
    features_enabled: List[str]
    features_disabled: List[str]
    test_results: Optional[Dict[str, Any]] = None
    
    # Release information
    release_notes: Optional[str] = None
    changelog: Optional[str] = None

class MetadataGenerator:
    def __init__(self, artifact_path: Path):
        self.artifact_path = Path(artifact_path)
        self.artifact_dir = self.artifact_path.parent if self.artifact_path.is_file() else self.artifact_path
        
    def generate_metadata(self, version: str = "unknown", build_id: str = "unknown") -> ArtifactMetadata:
        """Generate comprehensive metadata for artifact"""
        
        # Basic information
        name = self.artifact_path.name
        timestamp = datetime.now().isoformat()
        
        # Platform information
        target_platform, target_architecture = self._detect_target_platform()
        
        # Build environment
        build_env = self._collect_build_environment()
        
        # Git information
        git_info = self._collect_git_information()
        
        # Dependencies
        dependencies = self._collect_dependencies()
        
        # Binary information
        binaries = self._analyze_binaries()
        
        # Checksums
        checksums = self._calculate_checksums()
        
        # Build configuration
        build_flags = self._extract_build_flags()
        features_enabled, features_disabled = self._extract_features()
        
        return ArtifactMetadata(
            name=name,
            version=version,
            build_id=build_id,
            timestamp=timestamp,
            target_platform=target_platform,
            target_architecture=target_architecture,
            build_environment=build_env,
            git_info=git_info,
            dependencies=dependencies,
            binaries=binaries,
            checksums=checksums,
            build_flags=build_flags,
            features_enabled=features_enabled,
            features_disabled=features_disabled
        )
    
    def _detect_target_platform(self) -> tuple[str, str]:
        """Detect target platform and architecture"""
        # Try to detect from filename first
        filename = self.artifact_path.name.lower()
        
        if 'linux-x64' in filename or 'linux-amd64' in filename:
            return 'linux', 'x86_64'
        elif 'linux-arm64' in filename or 'linux-aarch64' in filename:
            return 'linux', 'aarch64'
        elif 'macos-x64' in filename or 'darwin-x64' in filename:
            return 'darwin', 'x86_64'
        elif 'macos-arm64' in filename or 'darwin-arm64' in filename:
            return 'darwin', 'aarch64'
        elif 'windows-x64' in filename or 'win64' in filename:
            return 'windows', 'x86_64'
        
        # Fall back to current platform
        system = platform_module.system().lower()
        machine = platform_module.machine()
        
        return system, machine
    
    def _collect_build_environment(self) -> BuildEnvironment:
        """Collect build environment information"""
        # OS information
        os_name = platform_module.system()
        os_version = platform_module.release()
        architecture = platform_module.machine()
        
        # Compiler information
        compiler, compiler_version = self._detect_compiler()
        
        # Build tools
        build_tools = self._detect_build_tools()
        
        # Environment variables
        env_vars = self._collect_environment_variables()
        
        return BuildEnvironment(
            os_name=os_name,
            os_version=os_version,
            architecture=architecture,
            compiler=compiler,
            compiler_version=compiler_version,
            build_tools=build_tools,
            environment_variables=env_vars
        )
    
    def _detect_compiler(self) -> tuple[str, str]:
        """Detect compiler and version"""
        # Check common compilers
        compilers = ['gcc', 'clang', 'cl']
        
        for compiler in compilers:
            try:
                result = subprocess.run(
                    [compiler, '--version'],
                    capture_output=True,
                    text=True,
                    timeout=5
                )
                if result.returncode == 0:
                    version_line = result.stdout.split('\n')[0]
                    return compiler, version_line
            except Exception:
                continue
        
        return 'unknown', 'unknown'
    
    def _detect_build_tools(self) -> Dict[str, str]:
        """Detect build tools and their versions"""
        tools = {
            'make': ['make', '--version'],
            'cmake': ['cmake', '--version'],
            'autoconf': ['autoconf', '--version'],
            'automake': ['automake', '--version'],
            'libtool': ['libtool', '--version'],
            'pkg-config': ['pkg-config', '--version'],
            'python': ['python3', '--version'],
            'git': ['git', '--version']
        }
        
        detected_tools = {}
        
        for tool, cmd in tools.items():
            try:
                result = subprocess.run(
                    cmd,
                    capture_output=True,
                    text=True,
                    timeout=5
                )
                if result.returncode == 0:
                    version_line = result.stdout.split('\n')[0]
                    detected_tools[tool] = version_line
            except Exception:
                detected_tools[tool] = 'not found'
        
        return detected_tools
    
    def _collect_environment_variables(self) -> Dict[str, str]:
        """Collect relevant environment variables"""
        relevant_vars = [
            'CC', 'CXX', 'AR', 'STRIP', 'RANLIB',
            'CFLAGS', 'CXXFLAGS', 'LDFLAGS',
            'PKG_CONFIG_PATH', 'PATH',
            'MACOSX_DEPLOYMENT_TARGET',
            'GITHUB_ACTIONS', 'GITHUB_WORKFLOW',
            'GITHUB_RUN_ID', 'GITHUB_RUN_NUMBER'
        ]
        
        env_vars = {}
        for var in relevant_vars:
            value = os.getenv(var)
            if value:
                # Truncate very long values (like PATH)
                if len(value) > 200:
                    value = value[:200] + "..."
                env_vars[var] = value
        
        return env_vars
    
    def _collect_git_information(self) -> GitInformation:
        """Collect git repository information"""
        try:
            # Get commit hash
            commit_hash = self._run_git_command(['rev-parse', 'HEAD'])
            
            # Get branch
            branch = self._run_git_command(['rev-parse', '--abbrev-ref', 'HEAD'])
            
            # Get tag (if on a tag)
            try:
                tag = self._run_git_command(['describe', '--exact-match', '--tags', 'HEAD'])
            except:
                tag = None
            
            # Get commit message
            commit_message = self._run_git_command(['log', '-1', '--pretty=format:%s'])
            
            # Get commit author
            commit_author = self._run_git_command(['log', '-1', '--pretty=format:%an <%ae>'])
            
            # Get commit date
            commit_date = self._run_git_command(['log', '-1', '--pretty=format:%ci'])
            
            # Get repository URL
            try:
                repository_url = self._run_git_command(['config', '--get', 'remote.origin.url'])
            except:
                repository_url = 'unknown'
            
            # Check if repository is dirty
            try:
                status_output = self._run_git_command(['status', '--porcelain'])
                is_dirty = bool(status_output.strip())
            except:
                is_dirty = False
            
            return GitInformation(
                commit_hash=commit_hash,
                branch=branch,
                tag=tag,
                commit_message=commit_message,
                commit_author=commit_author,
                commit_date=commit_date,
                repository_url=repository_url,
                is_dirty=is_dirty
            )
        
        except Exception as e:
            # Return minimal git info if git is not available
            return GitInformation(
                commit_hash=os.getenv('GITHUB_SHA', 'unknown'),
                branch=os.getenv('GITHUB_REF_NAME', 'unknown'),
                tag=None,
                commit_message='unknown',
                commit_author='unknown',
                commit_date='unknown',
                repository_url=f"https://github.com/{os.getenv('GITHUB_REPOSITORY', 'unknown')}",
                is_dirty=False
            )
    
    def _run_git_command(self, args: List[str]) -> str:
        """Run git command and return output"""
        result = subprocess.run(
            ['git'] + args,
            capture_output=True,
            text=True,
            timeout=10,
            cwd=self.artifact_dir
        )
        if result.returncode != 0:
            raise subprocess.CalledProcessError(result.returncode, args)
        return result.stdout.strip()
    
    def _collect_dependencies(self) -> List[DependencyInfo]:
        """Collect dependency information"""
        dependencies = []
        
        # Check for depends directory
        depends_dir = Path('depends')
        if depends_dir.exists():
            dependencies.extend(self._parse_depends_directory(depends_dir))
        
        # Check for package files
        package_files = ['package.json', 'Cargo.toml', 'requirements.txt', 'Pipfile']
        for package_file in package_files:
            if Path(package_file).exists():
                dependencies.extend(self._parse_package_file(Path(package_file)))
        
        return dependencies
    
    def _parse_depends_directory(self, depends_dir: Path) -> List[DependencyInfo]:
        """Parse dependencies from depends directory"""
        dependencies = []
        
        packages_dir = depends_dir / 'packages'
        if packages_dir.exists():
            for package_file in packages_dir.glob('*.mk'):
                try:
                    with open(package_file, 'r') as f:
                        content = f.read()
                    
                    # Extract package name and version
                    name = package_file.stem
                    version = 'unknown'
                    source = 'unknown'
                    
                    # Simple parsing of makefile variables
                    for line in content.split('\n'):
                        if line.startswith(f'{name}_version'):
                            version = line.split('=', 1)[1].strip()
                        elif line.startswith(f'{name}_download_path'):
                            source = line.split('=', 1)[1].strip()
                    
                    dependencies.append(DependencyInfo(
                        name=name,
                        version=version,
                        source=source
                    ))
                
                except Exception:
                    continue
        
        return dependencies
    
    def _parse_package_file(self, package_file: Path) -> List[DependencyInfo]:
        """Parse dependencies from package files"""
        dependencies = []
        
        try:
            if package_file.name == 'package.json':
                with open(package_file, 'r') as f:
                    data = json.load(f)
                
                for dep_type in ['dependencies', 'devDependencies']:
                    if dep_type in data:
                        for name, version in data[dep_type].items():
                            dependencies.append(DependencyInfo(
                                name=name,
                                version=version,
                                source='npm'
                            ))
        
        except Exception:
            pass
        
        return dependencies
    
    def _analyze_binaries(self) -> List[BinaryInfo]:
        """Analyze binary files in artifact"""
        binaries = []
        
        if self.artifact_path.is_file():
            binary_files = [self.artifact_path]
        else:
            # Find binary files in directory
            binary_extensions = ['.exe', '']  # Empty string for Unix executables
            binary_files = []
            
            for ext in binary_extensions:
                binary_files.extend(self.artifact_path.glob(f'*{ext}'))
            
            # Filter to actual executables
            binary_files = [f for f in binary_files if f.is_file() and os.access(f, os.X_OK)]
        
        for binary_file in binary_files:
            try:
                binary_info = self._analyze_single_binary(binary_file)
                binaries.append(binary_info)
            except Exception:
                continue
        
        return binaries
    
    def _analyze_single_binary(self, binary_path: Path) -> BinaryInfo:
        """Analyze a single binary file"""
        file_size = binary_path.stat().st_size
        
        # Get file information
        try:
            result = subprocess.run(
                ['file', str(binary_path)],
                capture_output=True,
                text=True,
                timeout=10
            )
            file_output = result.stdout if result.returncode == 0 else 'unknown'
        except:
            file_output = 'unknown'
        
        # Extract architecture and format
        architecture = self._extract_architecture_from_file_output(file_output)
        format_type = self._extract_format_from_file_output(file_output)
        
        # Check if stripped
        stripped = 'stripped' in file_output.lower()
        
        # Check if statically linked
        static_linked = 'statically linked' in file_output.lower()
        
        # Get dependencies
        dependencies = self._get_binary_dependencies(binary_path)
        
        return BinaryInfo(
            name=binary_path.name,
            size=file_size,
            architecture=architecture,
            format=format_type,
            stripped=stripped,
            static_linked=static_linked,
            dependencies=dependencies
        )
    
    def _extract_architecture_from_file_output(self, file_output: str) -> str:
        """Extract architecture from file command output"""
        file_output_lower = file_output.lower()
        
        if 'x86-64' in file_output_lower or 'x86_64' in file_output_lower:
            return 'x86_64'
        elif 'aarch64' in file_output_lower or 'arm64' in file_output_lower:
            return 'aarch64'
        elif 'arm' in file_output_lower:
            return 'arm'
        elif 'i386' in file_output_lower:
            return 'i386'
        else:
            return 'unknown'
    
    def _extract_format_from_file_output(self, file_output: str) -> str:
        """Extract format from file command output"""
        file_output_lower = file_output.lower()
        
        if 'elf' in file_output_lower:
            return 'ELF'
        elif 'mach-o' in file_output_lower:
            return 'Mach-O'
        elif 'pe32' in file_output_lower:
            return 'PE32'
        else:
            return 'unknown'
    
    def _get_binary_dependencies(self, binary_path: Path) -> List[str]:
        """Get binary dependencies"""
        dependencies = []
        
        try:
            system = platform_module.system().lower()
            
            if system == 'linux':
                result = subprocess.run(
                    ['ldd', str(binary_path)],
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
            
            elif system == 'darwin':
                result = subprocess.run(
                    ['otool', '-L', str(binary_path)],
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
        
        except Exception:
            pass
        
        return dependencies[:10]  # Limit to first 10 dependencies
    
    def _calculate_checksums(self) -> Dict[str, str]:
        """Calculate checksums for artifact"""
        import hashlib
        
        checksums = {}
        
        if self.artifact_path.is_file():
            files_to_hash = [self.artifact_path]
        else:
            files_to_hash = list(self.artifact_path.rglob('*'))
            files_to_hash = [f for f in files_to_hash if f.is_file()]
        
        for file_path in files_to_hash:
            try:
                with open(file_path, 'rb') as f:
                    content = f.read()
                
                relative_path = file_path.relative_to(self.artifact_path.parent)
                checksums[str(relative_path)] = {
                    'sha256': hashlib.sha256(content).hexdigest(),
                    'size': len(content)
                }
            except Exception:
                continue
        
        return checksums
    
    def _extract_build_flags(self) -> List[str]:
        """Extract build flags (placeholder implementation)"""
        # This would need to be implemented based on how build flags are stored
        # Could check config.log, build logs, or embedded information
        return []
    
    def _extract_features(self) -> tuple[List[str], List[str]]:
        """Extract enabled and disabled features"""
        # This would need to be implemented based on how features are configured
        # Could check configure output, config.h, or other configuration files
        enabled = []
        disabled = []
        
        # Check for common BabaChain features
        config_files = ['config.log', 'src/config/babachain-config.h']
        
        for config_file in config_files:
            if Path(config_file).exists():
                try:
                    with open(config_file, 'r') as f:
                        content = f.read()
                    
                    # Look for feature flags
                    if 'ENABLE_WALLET' in content:
                        enabled.append('wallet')
                    if 'ENABLE_ZMQ' in content:
                        enabled.append('zmq')
                    if 'ENABLE_UPNP' in content:
                        enabled.append('upnp')
                    
                except Exception:
                    continue
        
        return enabled, disabled

def main():
    if len(sys.argv) < 2:
        print("Usage: metadata-generator.py <artifact_path> [version] [build_id] [output_file]")
        sys.exit(1)
    
    artifact_path = Path(sys.argv[1])
    version = sys.argv[2] if len(sys.argv) > 2 else "unknown"
    build_id = sys.argv[3] if len(sys.argv) > 3 else os.getenv('GITHUB_RUN_ID', 'local')
    output_file = sys.argv[4] if len(sys.argv) > 4 else None
    
    if not artifact_path.exists():
        print(f"Error: Artifact not found: {artifact_path}")
        sys.exit(1)
    
    # Generate metadata
    generator = MetadataGenerator(artifact_path)
    metadata = generator.generate_metadata(version, build_id)
    
    # Convert to JSON
    metadata_json = json.dumps(asdict(metadata), indent=2, default=str)
    
    if output_file:
        with open(output_file, 'w') as f:
            f.write(metadata_json)
        print(f"Metadata saved to: {output_file}")
    else:
        print(metadata_json)

if __name__ == "__main__":
    main()