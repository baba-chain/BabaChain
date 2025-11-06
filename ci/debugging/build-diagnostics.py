#!/usr/bin/env python3
"""
BabaChain Build Environment Diagnostics
Comprehensive diagnostic tool for build environment issues
"""

import os
import sys
import json
import subprocess
import platform as platform_module
import shutil
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Any
from dataclasses import dataclass, asdict
from datetime import datetime

@dataclass
class DiagnosticResult:
    name: str
    category: str
    status: str  # pass, fail, warning, info
    message: str
    details: Optional[Dict[str, Any]] = None
    suggestion: Optional[str] = None

@dataclass
class SystemInfo:
    os_name: str
    os_version: str
    architecture: str
    hostname: str
    user: str
    shell: str
    python_version: str

@dataclass
class BuildEnvironmentDiagnostics:
    timestamp: str
    system_info: SystemInfo
    results: List[DiagnosticResult]
    summary: Dict[str, int]

class BuildDiagnostics:
    def __init__(self):
        self.results = []
        self.system_info = self._collect_system_info()
    
    def run_all_diagnostics(self) -> BuildEnvironmentDiagnostics:
        """Run all diagnostic checks"""
        self.results = []
        
        # System diagnostics
        self._check_system_requirements()
        self._check_disk_space()
        self._check_memory()
        self._check_network_connectivity()
        
        # Build tool diagnostics
        self._check_build_tools()
        self._check_compilers()
        self._check_autotools()
        self._check_pkg_config()
        
        # Platform-specific diagnostics
        self._check_platform_specific()
        
        # Project diagnostics
        self._check_project_structure()
        self._check_dependencies_system()
        self._check_git_repository()
        
        # Environment diagnostics
        self._check_environment_variables()
        self._check_path_configuration()
        
        # Generate summary
        summary = self._generate_summary()
        
        return BuildEnvironmentDiagnostics(
            timestamp=datetime.now().isoformat(),
            system_info=self.system_info,
            results=self.results,
            summary=summary
        )
    
    def _collect_system_info(self) -> SystemInfo:
        """Collect basic system information"""
        return SystemInfo(
            os_name=platform_module.system(),
            os_version=platform_module.release(),
            architecture=platform_module.machine(),
            hostname=platform_module.node(),
            user=os.getenv('USER', os.getenv('USERNAME', 'unknown')),
            shell=os.getenv('SHELL', 'unknown'),
            python_version=platform_module.python_version()
        )
    
    def _add_result(self, name: str, category: str, status: str, message: str,
                   details: Optional[Dict] = None, suggestion: Optional[str] = None):
        """Add diagnostic result"""
        self.results.append(DiagnosticResult(
            name=name,
            category=category,
            status=status,
            message=message,
            details=details,
            suggestion=suggestion
        ))
    
    def _check_system_requirements(self):
        """Check basic system requirements"""
        # Check OS support
        supported_os = ['Linux', 'Darwin', 'Windows']
        if self.system_info.os_name in supported_os:
            self._add_result(
                "os_support", "system", "pass",
                f"Operating system {self.system_info.os_name} is supported"
            )
        else:
            self._add_result(
                "os_support", "system", "warning",
                f"Operating system {self.system_info.os_name} may not be fully supported",
                suggestion="Consider using Linux, macOS, or Windows for best compatibility"
            )
        
        # Check architecture
        supported_archs = ['x86_64', 'aarch64', 'arm64', 'AMD64']
        if self.system_info.architecture in supported_archs:
            self._add_result(
                "architecture_support", "system", "pass",
                f"Architecture {self.system_info.architecture} is supported"
            )
        else:
            self._add_result(
                "architecture_support", "system", "warning",
                f"Architecture {self.system_info.architecture} may not be fully supported",
                suggestion="Consider using x86_64 or ARM64 architecture"
            )
    
    def _check_disk_space(self):
        """Check available disk space"""
        try:
            if hasattr(shutil, 'disk_usage'):
                total, used, free = shutil.disk_usage('.')
                free_gb = free / (1024**3)
                
                if free_gb < 1:
                    status = "fail"
                    message = f"Very low disk space: {free_gb:.1f} GB available"
                    suggestion = "Free up disk space before building"
                elif free_gb < 5:
                    status = "warning"
                    message = f"Low disk space: {free_gb:.1f} GB available"
                    suggestion = "Consider freeing up more disk space"
                else:
                    status = "pass"
                    message = f"Sufficient disk space: {free_gb:.1f} GB available"
                    suggestion = None
                
                self._add_result(
                    "disk_space", "system", status, message,
                    details={"free_gb": free_gb, "total_gb": total / (1024**3)},
                    suggestion=suggestion
                )
            else:
                self._add_result(
                    "disk_space", "system", "info",
                    "Disk space check not available on this platform"
                )
        except Exception as e:
            self._add_result(
                "disk_space", "system", "warning",
                f"Could not check disk space: {e}"
            )
    
    def _check_memory(self):
        """Check available memory"""
        try:
            if self.system_info.os_name == 'Linux':
                with open('/proc/meminfo', 'r') as f:
                    meminfo = f.read()
                
                for line in meminfo.split('\n'):
                    if line.startswith('MemAvailable:'):
                        available_kb = int(line.split()[1])
                        available_gb = available_kb / (1024**2)
                        
                        if available_gb < 1:
                            status = "fail"
                            message = f"Very low memory: {available_gb:.1f} GB available"
                            suggestion = "Close other applications or add more RAM"
                        elif available_gb < 2:
                            status = "warning"
                            message = f"Low memory: {available_gb:.1f} GB available"
                            suggestion = "Consider closing other applications"
                        else:
                            status = "pass"
                            message = f"Sufficient memory: {available_gb:.1f} GB available"
                            suggestion = None
                        
                        self._add_result(
                            "memory", "system", status, message,
                            details={"available_gb": available_gb},
                            suggestion=suggestion
                        )
                        break
            else:
                self._add_result(
                    "memory", "system", "info",
                    "Memory check not implemented for this platform"
                )
        except Exception as e:
            self._add_result(
                "memory", "system", "warning",
                f"Could not check memory: {e}"
            )
    
    def _check_network_connectivity(self):
        """Check network connectivity"""
        test_hosts = [
            ('github.com', 'GitHub (for source code)'),
            ('download.qt.io', 'Qt downloads'),
            ('ftp.gnu.org', 'GNU software')
        ]
        
        for host, description in test_hosts:
            try:
                result = subprocess.run(
                    ['ping', '-c', '1', host] if self.system_info.os_name != 'Windows' 
                    else ['ping', '-n', '1', host],
                    capture_output=True,
                    timeout=10
                )
                
                if result.returncode == 0:
                    self._add_result(
                        f"network_{host.replace('.', '_')}", "network", "pass",
                        f"Can reach {description}"
                    )
                else:
                    self._add_result(
                        f"network_{host.replace('.', '_')}", "network", "warning",
                        f"Cannot reach {description}",
                        suggestion="Check network connectivity and firewall settings"
                    )
            except Exception:
                self._add_result(
                    f"network_{host.replace('.', '_')}", "network", "warning",
                    f"Network test failed for {description}"
                )
    
    def _check_build_tools(self):
        """Check essential build tools"""
        essential_tools = {
            'make': 'Build automation tool',
            'git': 'Version control system',
            'python3': 'Python interpreter',
            'curl': 'Download tool',
            'tar': 'Archive tool',
            'unzip': 'Archive tool'
        }
        
        for tool, description in essential_tools.items():
            if shutil.which(tool):
                try:
                    result = subprocess.run(
                        [tool, '--version'],
                        capture_output=True,
                        text=True,
                        timeout=5
                    )
                    version = result.stdout.split('\n')[0] if result.returncode == 0 else 'unknown'
                    
                    self._add_result(
                        f"tool_{tool}", "build_tools", "pass",
                        f"{description} is available",
                        details={"version": version}
                    )
                except Exception:
                    self._add_result(
                        f"tool_{tool}", "build_tools", "pass",
                        f"{description} is available (version check failed)"
                    )
            else:
                self._add_result(
                    f"tool_{tool}", "build_tools", "fail",
                    f"{description} is not available",
                    suggestion=f"Install {tool} using your system package manager"
                )
    
    def _check_compilers(self):
        """Check available compilers"""
        compilers = {
            'gcc': 'GNU Compiler Collection',
            'g++': 'GNU C++ Compiler',
            'clang': 'Clang C Compiler',
            'clang++': 'Clang C++ Compiler'
        }
        
        found_c_compiler = False
        found_cpp_compiler = False
        
        for compiler, description in compilers.items():
            if shutil.which(compiler):
                try:
                    result = subprocess.run(
                        [compiler, '--version'],
                        capture_output=True,
                        text=True,
                        timeout=5
                    )
                    version = result.stdout.split('\n')[0] if result.returncode == 0 else 'unknown'
                    
                    self._add_result(
                        f"compiler_{compiler}", "compilers", "pass",
                        f"{description} is available",
                        details={"version": version}
                    )
                    
                    if compiler in ['gcc', 'clang']:
                        found_c_compiler = True
                    if compiler in ['g++', 'clang++']:
                        found_cpp_compiler = True
                        
                except Exception:
                    self._add_result(
                        f"compiler_{compiler}", "compilers", "warning",
                        f"{description} found but version check failed"
                    )
        
        # Check if we have at least one C and C++ compiler
        if not found_c_compiler:
            self._add_result(
                "c_compiler", "compilers", "fail",
                "No C compiler found",
                suggestion="Install gcc or clang"
            )
        
        if not found_cpp_compiler:
            self._add_result(
                "cpp_compiler", "compilers", "fail",
                "No C++ compiler found",
                suggestion="Install g++ or clang++"
            )
    
    def _check_autotools(self):
        """Check autotools availability"""
        autotools = {
            'autoconf': 'GNU Autoconf',
            'automake': 'GNU Automake',
            'libtool': 'GNU Libtool',
            'pkg-config': 'Package Config'
        }
        
        for tool, description in autotools.items():
            if shutil.which(tool):
                try:
                    result = subprocess.run(
                        [tool, '--version'],
                        capture_output=True,
                        text=True,
                        timeout=5
                    )
                    version = result.stdout.split('\n')[0] if result.returncode == 0 else 'unknown'
                    
                    self._add_result(
                        f"autotool_{tool}", "autotools", "pass",
                        f"{description} is available",
                        details={"version": version}
                    )
                except Exception:
                    self._add_result(
                        f"autotool_{tool}", "autotools", "warning",
                        f"{description} found but version check failed"
                    )
            else:
                self._add_result(
                    f"autotool_{tool}", "autotools", "fail",
                    f"{description} is not available",
                    suggestion=f"Install {tool} using your system package manager"
                )
    
    def _check_pkg_config(self):
        """Check pkg-config configuration"""
        if shutil.which('pkg-config'):
            # Check PKG_CONFIG_PATH
            pkg_config_path = os.getenv('PKG_CONFIG_PATH', '')
            
            if pkg_config_path:
                paths = pkg_config_path.split(':' if self.system_info.os_name != 'Windows' else ';')
                valid_paths = [p for p in paths if Path(p).exists()]
                
                self._add_result(
                    "pkg_config_path", "autotools", "info",
                    f"PKG_CONFIG_PATH has {len(valid_paths)} valid paths",
                    details={"paths": valid_paths}
                )
            else:
                self._add_result(
                    "pkg_config_path", "autotools", "warning",
                    "PKG_CONFIG_PATH is not set",
                    suggestion="Set PKG_CONFIG_PATH if you have custom libraries"
                )
            
            # Test pkg-config functionality
            try:
                result = subprocess.run(
                    ['pkg-config', '--list-all'],
                    capture_output=True,
                    text=True,
                    timeout=10
                )
                
                if result.returncode == 0:
                    package_count = len(result.stdout.strip().split('\n'))
                    self._add_result(
                        "pkg_config_packages", "autotools", "pass",
                        f"pkg-config found {package_count} packages"
                    )
                else:
                    self._add_result(
                        "pkg_config_packages", "autotools", "warning",
                        "pkg-config --list-all failed"
                    )
            except Exception:
                self._add_result(
                    "pkg_config_packages", "autotools", "warning",
                    "Could not test pkg-config functionality"
                )
    
    def _check_platform_specific(self):
        """Check platform-specific requirements"""
        if self.system_info.os_name == 'Darwin':  # macOS
            self._check_macos_specific()
        elif self.system_info.os_name == 'Linux':
            self._check_linux_specific()
        elif self.system_info.os_name == 'Windows':
            self._check_windows_specific()
    
    def _check_macos_specific(self):
        """Check macOS-specific requirements"""
        # Check Xcode command line tools
        if Path('/usr/bin/xcode-select').exists():
            try:
                result = subprocess.run(
                    ['xcode-select', '-p'],
                    capture_output=True,
                    text=True,
                    timeout=5
                )
                
                if result.returncode == 0:
                    self._add_result(
                        "xcode_tools", "platform", "pass",
                        "Xcode command line tools are installed",
                        details={"path": result.stdout.strip()}
                    )
                else:
                    self._add_result(
                        "xcode_tools", "platform", "fail",
                        "Xcode command line tools are not properly configured",
                        suggestion="Run: xcode-select --install"
                    )
            except Exception:
                self._add_result(
                    "xcode_tools", "platform", "warning",
                    "Could not check Xcode command line tools"
                )
        
        # Check SDK
        try:
            result = subprocess.run(
                ['xcrun', '--show-sdk-path'],
                capture_output=True,
                text=True,
                timeout=5
            )
            
            if result.returncode == 0:
                sdk_path = result.stdout.strip()
                self._add_result(
                    "macos_sdk", "platform", "pass",
                    "macOS SDK is available",
                    details={"sdk_path": sdk_path}
                )
            else:
                self._add_result(
                    "macos_sdk", "platform", "fail",
                    "macOS SDK is not available",
                    suggestion="Install Xcode or command line tools"
                )
        except Exception:
            self._add_result(
                "macos_sdk", "platform", "warning",
                "Could not check macOS SDK"
            )
        
        # Check Homebrew
        if shutil.which('brew'):
            self._add_result(
                "homebrew", "platform", "pass",
                "Homebrew is available"
            )
        else:
            self._add_result(
                "homebrew", "platform", "info",
                "Homebrew is not installed",
                suggestion="Consider installing Homebrew for easier dependency management"
            )
    
    def _check_linux_specific(self):
        """Check Linux-specific requirements"""
        # Check distribution
        try:
            if Path('/etc/os-release').exists():
                with open('/etc/os-release', 'r') as f:
                    os_release = f.read()
                
                distro_info = {}
                for line in os_release.split('\n'):
                    if '=' in line:
                        key, value = line.split('=', 1)
                        distro_info[key] = value.strip('"')
                
                self._add_result(
                    "linux_distro", "platform", "info",
                    f"Linux distribution: {distro_info.get('NAME', 'unknown')}",
                    details=distro_info
                )
        except Exception:
            self._add_result(
                "linux_distro", "platform", "info",
                "Could not determine Linux distribution"
            )
        
        # Check package managers
        package_managers = ['apt', 'yum', 'dnf', 'pacman', 'zypper']
        found_pm = False
        
        for pm in package_managers:
            if shutil.which(pm):
                self._add_result(
                    f"package_manager_{pm}", "platform", "pass",
                    f"Package manager {pm} is available"
                )
                found_pm = True
        
        if not found_pm:
            self._add_result(
                "package_manager", "platform", "warning",
                "No recognized package manager found"
            )
    
    def _check_windows_specific(self):
        """Check Windows-specific requirements"""
        # Check for MSYS2
        if os.getenv('MSYSTEM'):
            self._add_result(
                "msys2", "platform", "pass",
                f"MSYS2 environment detected: {os.getenv('MSYSTEM')}"
            )
        else:
            self._add_result(
                "msys2", "platform", "warning",
                "MSYS2 environment not detected",
                suggestion="Use MSYS2 for building on Windows"
            )
        
        # Check for Visual Studio
        vs_paths = [
            'C:\\Program Files\\Microsoft Visual Studio',
            'C:\\Program Files (x86)\\Microsoft Visual Studio'
        ]
        
        found_vs = False
        for vs_path in vs_paths:
            if Path(vs_path).exists():
                found_vs = True
                break
        
        if found_vs:
            self._add_result(
                "visual_studio", "platform", "info",
                "Visual Studio installation detected"
            )
        else:
            self._add_result(
                "visual_studio", "platform", "info",
                "Visual Studio not detected (not required for MinGW builds)"
            )
    
    def _check_project_structure(self):
        """Check project structure"""
        required_files = [
            ('configure.ac', 'Autoconf configuration'),
            ('Makefile.am', 'Automake configuration'),
            ('src/', 'Source directory'),
            ('depends/', 'Dependencies directory')
        ]
        
        for file_path, description in required_files:
            if Path(file_path).exists():
                self._add_result(
                    f"project_{file_path.replace('/', '_').replace('.', '_')}", 
                    "project", "pass",
                    f"{description} exists"
                )
            else:
                status = "fail" if file_path in ['configure.ac', 'src/'] else "warning"
                self._add_result(
                    f"project_{file_path.replace('/', '_').replace('.', '_')}", 
                    "project", status,
                    f"{description} is missing",
                    suggestion=f"Ensure {file_path} exists in the project root"
                )
        
        # Check if configure script exists
        if Path('configure').exists():
            self._add_result(
                "configure_script", "project", "pass",
                "Configure script exists"
            )
        elif Path('configure.ac').exists():
            self._add_result(
                "configure_script", "project", "info",
                "Configure script not found, but configure.ac exists",
                suggestion="Run ./autogen.sh to generate configure script"
            )
        else:
            self._add_result(
                "configure_script", "project", "fail",
                "Neither configure script nor configure.ac found"
            )
    
    def _check_dependencies_system(self):
        """Check dependencies system"""
        depends_dir = Path('depends')
        
        if not depends_dir.exists():
            self._add_result(
                "depends_directory", "dependencies", "warning",
                "Dependencies directory not found"
            )
            return
        
        # Check depends structure
        required_subdirs = ['packages', 'hosts', 'builders']
        for subdir in required_subdirs:
            subdir_path = depends_dir / subdir
            if subdir_path.exists():
                self._add_result(
                    f"depends_{subdir}", "dependencies", "pass",
                    f"Dependencies {subdir} directory exists"
                )
            else:
                self._add_result(
                    f"depends_{subdir}", "dependencies", "warning",
                    f"Dependencies {subdir} directory is missing"
                )
        
        # Check for built dependencies
        built_dir = depends_dir / 'built'
        if built_dir.exists():
            built_count = len(list(built_dir.iterdir()))
            self._add_result(
                "depends_built", "dependencies", "info",
                f"Found {built_count} built dependencies"
            )
        else:
            self._add_result(
                "depends_built", "dependencies", "info",
                "No built dependencies found (will be built during first build)"
            )
    
    def _check_git_repository(self):
        """Check git repository status"""
        if not Path('.git').exists():
            self._add_result(
                "git_repository", "git", "warning",
                "Not in a git repository"
            )
            return
        
        try:
            # Check git status
            result = subprocess.run(
                ['git', 'status', '--porcelain'],
                capture_output=True,
                text=True,
                timeout=5
            )
            
            if result.returncode == 0:
                if result.stdout.strip():
                    self._add_result(
                        "git_status", "git", "info",
                        "Repository has uncommitted changes"
                    )
                else:
                    self._add_result(
                        "git_status", "git", "pass",
                        "Repository is clean"
                    )
            
            # Check current branch
            result = subprocess.run(
                ['git', 'rev-parse', '--abbrev-ref', 'HEAD'],
                capture_output=True,
                text=True,
                timeout=5
            )
            
            if result.returncode == 0:
                branch = result.stdout.strip()
                self._add_result(
                    "git_branch", "git", "info",
                    f"Current branch: {branch}"
                )
            
            # Check for submodules
            if Path('.gitmodules').exists():
                result = subprocess.run(
                    ['git', 'submodule', 'status'],
                    capture_output=True,
                    text=True,
                    timeout=10
                )
                
                if result.returncode == 0:
                    submodules = result.stdout.strip().split('\n')
                    uninitialized = [s for s in submodules if s.startswith('-')]
                    
                    if uninitialized:
                        self._add_result(
                            "git_submodules", "git", "warning",
                            f"{len(uninitialized)} submodules are not initialized",
                            suggestion="Run: git submodule update --init --recursive"
                        )
                    else:
                        self._add_result(
                            "git_submodules", "git", "pass",
                            "All submodules are initialized"
                        )
        
        except Exception as e:
            self._add_result(
                "git_repository", "git", "warning",
                f"Could not check git repository: {e}"
            )
    
    def _check_environment_variables(self):
        """Check important environment variables"""
        important_vars = {
            'CC': 'C compiler',
            'CXX': 'C++ compiler',
            'CFLAGS': 'C compiler flags',
            'CXXFLAGS': 'C++ compiler flags',
            'LDFLAGS': 'Linker flags',
            'PKG_CONFIG_PATH': 'pkg-config search path',
            'PATH': 'Executable search path'
        }
        
        for var, description in important_vars.items():
            value = os.getenv(var)
            if value:
                # Truncate very long values
                display_value = value if len(value) <= 100 else value[:100] + "..."
                self._add_result(
                    f"env_{var.lower()}", "environment", "info",
                    f"{description} is set",
                    details={"value": display_value}
                )
            else:
                status = "warning" if var in ['CC', 'CXX'] else "info"
                self._add_result(
                    f"env_{var.lower()}", "environment", status,
                    f"{description} is not set"
                )
    
    def _check_path_configuration(self):
        """Check PATH configuration"""
        path = os.getenv('PATH', '')
        if not path:
            self._add_result(
                "path_empty", "environment", "fail",
                "PATH environment variable is empty"
            )
            return
        
        paths = path.split(':' if self.system_info.os_name != 'Windows' else ';')
        valid_paths = [p for p in paths if Path(p).exists()]
        
        self._add_result(
            "path_validity", "environment", "info",
            f"PATH has {len(valid_paths)} valid directories out of {len(paths)}",
            details={"valid_count": len(valid_paths), "total_count": len(paths)}
        )
        
        # Check for common tool directories
        common_dirs = ['/usr/bin', '/usr/local/bin', '/opt/homebrew/bin']
        found_dirs = [d for d in common_dirs if d in paths and Path(d).exists()]
        
        if found_dirs:
            self._add_result(
                "path_common_dirs", "environment", "pass",
                f"PATH includes common tool directories: {', '.join(found_dirs)}"
            )
    
    def _generate_summary(self) -> Dict[str, int]:
        """Generate summary of results"""
        summary = {"pass": 0, "fail": 0, "warning": 0, "info": 0}
        
        for result in self.results:
            summary[result.status] += 1
        
        return summary

def main():
    if len(sys.argv) > 1 and sys.argv[1] in ['-h', '--help']:
        print("BabaChain Build Environment Diagnostics")
        print("Usage: build-diagnostics.py [output_format]")
        print("Output formats: json, markdown, text (default: text)")
        sys.exit(0)
    
    output_format = sys.argv[1] if len(sys.argv) > 1 else "text"
    
    # Run diagnostics
    diagnostics = BuildDiagnostics()
    report = diagnostics.run_all_diagnostics()
    
    # Output based on format
    if output_format == "json":
        print(json.dumps(asdict(report), indent=2, default=str))
    
    elif output_format == "markdown":
        print("# Build Environment Diagnostics Report\n")
        print(f"**Timestamp:** {report.timestamp}")
        print(f"**System:** {report.system_info.os_name} {report.system_info.os_version} ({report.system_info.architecture})")
        print(f"**User:** {report.system_info.user}@{report.system_info.hostname}\n")
        
        print("## Summary")
        print(f"- ✅ Passed: {report.summary['pass']}")
        print(f"- ⚠️ Warnings: {report.summary['warning']}")
        print(f"- ❌ Failed: {report.summary['fail']}")
        print(f"- ℹ️ Info: {report.summary['info']}\n")
        
        # Group results by category
        categories = {}
        for result in report.results:
            if result.category not in categories:
                categories[result.category] = []
            categories[result.category].append(result)
        
        for category, results in categories.items():
            print(f"## {category.title()}")
            for result in results:
                icon = {"pass": "✅", "warning": "⚠️", "fail": "❌", "info": "ℹ️"}[result.status]
                print(f"- {icon} **{result.name}**: {result.message}")
                if result.suggestion:
                    print(f"  - *Suggestion: {result.suggestion}*")
            print()
    
    else:  # text format
        print("BabaChain Build Environment Diagnostics Report")
        print("=" * 60)
        print(f"Timestamp: {report.timestamp}")
        print(f"System: {report.system_info.os_name} {report.system_info.os_version} ({report.system_info.architecture})")
        print(f"User: {report.system_info.user}@{report.system_info.hostname}")
        print()
        
        print("Summary:")
        print(f"  Passed: {report.summary['pass']}")
        print(f"  Warnings: {report.summary['warning']}")
        print(f"  Failed: {report.summary['fail']}")
        print(f"  Info: {report.summary['info']}")
        print()
        
        # Show failures and warnings first
        critical_results = [r for r in report.results if r.status in ['fail', 'warning']]
        if critical_results:
            print("Critical Issues:")
            for result in critical_results:
                status_symbol = "❌" if result.status == "fail" else "⚠️"
                print(f"  {status_symbol} [{result.category}] {result.name}: {result.message}")
                if result.suggestion:
                    print(f"      Suggestion: {result.suggestion}")
            print()
        
        # Show all results grouped by category
        categories = {}
        for result in report.results:
            if result.category not in categories:
                categories[result.category] = []
            categories[result.category].append(result)
        
        for category, results in categories.items():
            print(f"{category.title()}:")
            for result in results:
                status_map = {"pass": "✓", "warning": "⚠", "fail": "✗", "info": "i"}
                print(f"  [{status_map[result.status]}] {result.name}: {result.message}")
            print()
    
    # Exit with error code if there are failures
    sys.exit(1 if report.summary['fail'] > 0 else 0)

if __name__ == "__main__":
    main()