#!/usr/bin/env python3
"""
BabaChain Platform-Specific Troubleshooting Guide Generator
Generates interactive troubleshooting guides for different platforms
"""

import os
import sys
import json
import platform as platform_module
import subprocess
from pathlib import Path
from typing import Dict, List, Optional, Any
from dataclasses import dataclass, asdict
from datetime import datetime

@dataclass
class TroubleshootingStep:
    title: str
    description: str
    commands: List[str]
    expected_output: Optional[str] = None
    common_issues: List[str] = None
    solutions: List[str] = None

@dataclass
class TroubleshootingSection:
    name: str
    description: str
    steps: List[TroubleshootingStep]
    prerequisites: List[str] = None

@dataclass
class PlatformGuide:
    platform: str
    version: str
    timestamp: str
    sections: List[TroubleshootingSection]
    common_issues: Dict[str, str]
    useful_links: Dict[str, str]

class TroubleshootingGuideGenerator:
    def __init__(self):
        self.platform = platform_module.system()
        self.architecture = platform_module.machine()
        
    def generate_guide(self, platform_override: str = None) -> PlatformGuide:
        """Generate platform-specific troubleshooting guide"""
        target_platform = platform_override or self.platform.lower()
        
        if target_platform in ['darwin', 'macos']:
            return self._generate_macos_guide()
        elif target_platform == 'linux':
            return self._generate_linux_guide()
        elif target_platform in ['windows', 'win32']:
            return self._generate_windows_guide()
        else:
            return self._generate_generic_guide()
    
    def _generate_macos_guide(self) -> PlatformGuide:
        """Generate macOS-specific troubleshooting guide"""
        sections = [
            self._macos_xcode_section(),
            self._macos_homebrew_section(),
            self._macos_dependencies_section(),
            self._macos_build_section(),
            self._macos_common_errors_section()
        ]
        
        common_issues = {
            "Command Line Tools not installed": "Run: xcode-select --install",
            "SDK not found": "Install Xcode or update command line tools",
            "Homebrew not found": "Install Homebrew from https://brew.sh",
            "Permission denied": "Check file permissions and ownership",
            "Library not found": "Check library paths and PKG_CONFIG_PATH",
            "Architecture mismatch": "Ensure you're building for the correct architecture (Intel vs Apple Silicon)"
        }
        
        useful_links = {
            "Xcode Command Line Tools": "https://developer.apple.com/xcode/",
            "Homebrew": "https://brew.sh",
            "macOS Build Guide": "https://github.com/Baba-Chain/BabaChain/blob/main/doc/build-osx.md",
            "Apple Developer Documentation": "https://developer.apple.com/documentation/"
        }
        
        return PlatformGuide(
            platform="macOS",
            version=platform_module.mac_ver()[0],
            timestamp=datetime.now().isoformat(),
            sections=sections,
            common_issues=common_issues,
            useful_links=useful_links
        )
    
    def _generate_linux_guide(self) -> PlatformGuide:
        """Generate Linux-specific troubleshooting guide"""
        sections = [
            self._linux_system_info_section(),
            self._linux_package_manager_section(),
            self._linux_dependencies_section(),
            self._linux_build_section(),
            self._linux_common_errors_section()
        ]
        
        common_issues = {
            "Package not found": "Update package lists and install missing packages",
            "Permission denied": "Use sudo for system-wide installations or check file permissions",
            "Library not found": "Install development packages (-dev or -devel)",
            "Compiler not found": "Install build-essential or development tools",
            "Configure script fails": "Install autotools and required dependencies",
            "Cross-compilation fails": "Install cross-compilation toolchain"
        }
        
        useful_links = {
            "Ubuntu Build Guide": "https://github.com/Baba-Chain/BabaChain/blob/main/doc/build-unix.md",
            "Debian Packages": "https://packages.debian.org/",
            "Ubuntu Packages": "https://packages.ubuntu.com/",
            "CentOS/RHEL Packages": "https://centos.pkgs.org/"
        }
        
        return PlatformGuide(
            platform="Linux",
            version=platform_module.release(),
            timestamp=datetime.now().isoformat(),
            sections=sections,
            common_issues=common_issues,
            useful_links=useful_links
        )
    
    def _generate_windows_guide(self) -> PlatformGuide:
        """Generate Windows-specific troubleshooting guide"""
        sections = [
            self._windows_msys2_section(),
            self._windows_mingw_section(),
            self._windows_dependencies_section(),
            self._windows_build_section(),
            self._windows_common_errors_section()
        ]
        
        common_issues = {
            "MSYS2 not found": "Install MSYS2 from https://www.msys2.org/",
            "MinGW not found": "Install MinGW toolchain in MSYS2",
            "Path issues": "Ensure MSYS2 paths are correctly set",
            "Permission denied": "Run as administrator or check file permissions",
            "DLL not found": "Check PATH and ensure all required DLLs are available",
            "Build tools not found": "Install build tools in MSYS2 environment"
        }
        
        useful_links = {
            "MSYS2": "https://www.msys2.org/",
            "MinGW-w64": "https://www.mingw-w64.org/",
            "Windows Build Guide": "https://github.com/Baba-Chain/BabaChain/blob/main/doc/build-windows.md",
            "Visual Studio": "https://visualstudio.microsoft.com/"
        }
        
        return PlatformGuide(
            platform="Windows",
            version=platform_module.release(),
            timestamp=datetime.now().isoformat(),
            sections=sections,
            common_issues=common_issues,
            useful_links=useful_links
        )
    
    def _generate_generic_guide(self) -> PlatformGuide:
        """Generate generic troubleshooting guide"""
        sections = [
            self._generic_system_section(),
            self._generic_build_tools_section(),
            self._generic_dependencies_section(),
            self._generic_build_section()
        ]
        
        return PlatformGuide(
            platform="Generic",
            version="unknown",
            timestamp=datetime.now().isoformat(),
            sections=sections,
            common_issues={},
            useful_links={}
        )
    
    # macOS-specific sections
    def _macos_xcode_section(self) -> TroubleshootingSection:
        return TroubleshootingSection(
            name="Xcode Command Line Tools",
            description="Verify and install Xcode command line tools",
            steps=[
                TroubleshootingStep(
                    title="Check if Xcode tools are installed",
                    description="Verify that Xcode command line tools are properly installed",
                    commands=["xcode-select -p"],
                    expected_output="/Applications/Xcode.app/Contents/Developer or /Library/Developer/CommandLineTools",
                    common_issues=["Command not found", "No developer directory"],
                    solutions=["Install Xcode command line tools: xcode-select --install"]
                ),
                TroubleshootingStep(
                    title="Check SDK availability",
                    description="Verify that macOS SDK is available",
                    commands=["xcrun --show-sdk-path"],
                    expected_output="Path to macOS SDK",
                    common_issues=["SDK not found", "Invalid SDK path"],
                    solutions=["Update Xcode or command line tools", "Reset Xcode path: sudo xcode-select --reset"]
                ),
                TroubleshootingStep(
                    title="Verify compiler availability",
                    description="Check that clang compiler is available",
                    commands=["clang --version", "clang++ --version"],
                    expected_output="Apple clang version information",
                    common_issues=["Compiler not found"],
                    solutions=["Install or update Xcode command line tools"]
                )
            ]
        )
    
    def _macos_homebrew_section(self) -> TroubleshootingSection:
        return TroubleshootingSection(
            name="Homebrew Package Manager",
            description="Set up and verify Homebrew for dependency management",
            steps=[
                TroubleshootingStep(
                    title="Check Homebrew installation",
                    description="Verify Homebrew is installed and working",
                    commands=["brew --version", "brew doctor"],
                    expected_output="Homebrew version and health check",
                    common_issues=["Command not found", "Homebrew warnings"],
                    solutions=[
                        "Install Homebrew: /bin/bash -c \"$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\"",
                        "Fix Homebrew issues as suggested by 'brew doctor'"
                    ]
                ),
                TroubleshootingStep(
                    title="Install build dependencies",
                    description="Install required build tools via Homebrew",
                    commands=[
                        "brew install automake libtool pkg-config",
                        "brew install python3"
                    ],
                    common_issues=["Package conflicts", "Permission errors"],
                    solutions=["Update Homebrew: brew update", "Fix permissions: sudo chown -R $(whoami) /usr/local"]
                )
            ]
        )
    
    def _macos_dependencies_section(self) -> TroubleshootingSection:
        return TroubleshootingSection(
            name="Dependencies Management",
            description="Handle BabaChain dependencies on macOS",
            steps=[
                TroubleshootingStep(
                    title="Build dependencies using depends system",
                    description="Use BabaChain's built-in dependency system",
                    commands=[
                        "cd depends",
                        "make HOST=x86_64-apple-darwin19 -j4"
                    ],
                    common_issues=["Download failures", "Build errors", "Checksum mismatches"],
                    solutions=[
                        "Check network connectivity",
                        "Clear depends cache: rm -rf built sources",
                        "Update dependency checksums"
                    ]
                )
            ]
        )
    
    def _macos_build_section(self) -> TroubleshootingSection:
        return TroubleshootingSection(
            name="Building BabaChain",
            description="Build BabaChain on macOS",
            steps=[
                TroubleshootingStep(
                    title="Configure build",
                    description="Run autotools configuration",
                    commands=[
                        "./autogen.sh",
                        "CONFIG_SITE=$PWD/depends/x86_64-apple-darwin19/share/config.site ./configure --prefix=/"
                    ],
                    common_issues=["Configure script not found", "Dependencies not found"],
                    solutions=["Run autogen.sh first", "Build dependencies first"]
                ),
                TroubleshootingStep(
                    title="Compile BabaChain",
                    description="Compile the BabaChain binaries",
                    commands=["make -j4"],
                    common_issues=["Compilation errors", "Linker errors"],
                    solutions=["Check compiler errors", "Verify all dependencies are built"]
                )
            ]
        )
    
    def _macos_common_errors_section(self) -> TroubleshootingSection:
        return TroubleshootingSection(
            name="Common macOS Build Errors",
            description="Solutions for common macOS-specific build issues",
            steps=[
                TroubleshootingStep(
                    title="Fix architecture mismatch",
                    description="Handle Intel vs Apple Silicon architecture issues",
                    commands=["arch", "uname -m"],
                    common_issues=["Wrong architecture", "Universal binary issues"],
                    solutions=[
                        "For Apple Silicon: use arm64 targets",
                        "For Intel: use x86_64 targets",
                        "Check ARCHFLAGS environment variable"
                    ]
                ),
                TroubleshootingStep(
                    title="Fix library path issues",
                    description="Resolve library linking problems",
                    commands=["otool -L <binary>", "install_name_tool -change <old> <new> <binary>"],
                    common_issues=["Library not found", "Wrong library paths"],
                    solutions=["Set DYLD_LIBRARY_PATH", "Use install_name_tool to fix paths"]
                )
            ]
        )
    
    # Linux-specific sections
    def _linux_system_info_section(self) -> TroubleshootingSection:
        return TroubleshootingSection(
            name="System Information",
            description="Gather Linux system information for troubleshooting",
            steps=[
                TroubleshootingStep(
                    title="Check distribution and version",
                    description="Identify Linux distribution and version",
                    commands=["cat /etc/os-release", "lsb_release -a", "uname -a"],
                    expected_output="Distribution name, version, and kernel information"
                ),
                TroubleshootingStep(
                    title="Check architecture",
                    description="Verify system architecture",
                    commands=["uname -m", "arch"],
                    expected_output="x86_64, aarch64, or other architecture"
                )
            ]
        )
    
    def _linux_package_manager_section(self) -> TroubleshootingSection:
        return TroubleshootingSection(
            name="Package Manager Setup",
            description="Configure package manager for dependency installation",
            steps=[
                TroubleshootingStep(
                    title="Update package lists",
                    description="Update package manager databases",
                    commands=[
                        "sudo apt update  # Ubuntu/Debian",
                        "sudo yum update  # CentOS/RHEL",
                        "sudo dnf update  # Fedora"
                    ]
                ),
                TroubleshootingStep(
                    title="Install build essentials",
                    description="Install basic build tools",
                    commands=[
                        "sudo apt install build-essential  # Ubuntu/Debian",
                        "sudo yum groupinstall 'Development Tools'  # CentOS/RHEL",
                        "sudo dnf groupinstall 'Development Tools'  # Fedora"
                    ]
                )
            ]
        )
    
    def _linux_dependencies_section(self) -> TroubleshootingSection:
        return TroubleshootingSection(
            name="Dependencies Installation",
            description="Install BabaChain build dependencies on Linux",
            steps=[
                TroubleshootingStep(
                    title="Install autotools",
                    description="Install autoconf, automake, and libtool",
                    commands=[
                        "sudo apt install autoconf automake libtool pkg-config  # Ubuntu/Debian",
                        "sudo yum install autoconf automake libtool pkgconfig  # CentOS/RHEL"
                    ]
                ),
                TroubleshootingStep(
                    title="Install additional dependencies",
                    description="Install other required packages",
                    commands=[
                        "sudo apt install python3 curl ca-certificates  # Ubuntu/Debian",
                        "sudo yum install python3 curl ca-certificates  # CentOS/RHEL"
                    ]
                )
            ]
        )
    
    def _linux_build_section(self) -> TroubleshootingSection:
        return TroubleshootingSection(
            name="Building on Linux",
            description="Build BabaChain on Linux systems",
            steps=[
                TroubleshootingStep(
                    title="Build dependencies",
                    description="Build dependencies using the depends system",
                    commands=[
                        "cd depends",
                        "make HOST=x86_64-pc-linux-gnu -j$(nproc)"
                    ]
                ),
                TroubleshootingStep(
                    title="Configure and build",
                    description="Configure and compile BabaChain",
                    commands=[
                        "./autogen.sh",
                        "CONFIG_SITE=$PWD/depends/x86_64-pc-linux-gnu/share/config.site ./configure --prefix=/",
                        "make -j$(nproc)"
                    ]
                )
            ]
        )
    
    def _linux_common_errors_section(self) -> TroubleshootingSection:
        return TroubleshootingSection(
            name="Common Linux Build Errors",
            description="Solutions for common Linux build issues",
            steps=[
                TroubleshootingStep(
                    title="Fix missing development packages",
                    description="Install missing -dev or -devel packages",
                    commands=["apt search <package>-dev", "yum search <package>-devel"],
                    solutions=["Install development versions of libraries"]
                ),
                TroubleshootingStep(
                    title="Fix permission issues",
                    description="Handle file permission problems",
                    commands=["ls -la", "chmod +x <file>", "chown <user>:<group> <file>"],
                    solutions=["Fix file permissions", "Use sudo for system operations"]
                )
            ]
        )
    
    # Windows-specific sections
    def _windows_msys2_section(self) -> TroubleshootingSection:
        return TroubleshootingSection(
            name="MSYS2 Environment",
            description="Set up MSYS2 for Windows builds",
            steps=[
                TroubleshootingStep(
                    title="Install MSYS2",
                    description="Download and install MSYS2",
                    commands=["Download from https://www.msys2.org/"],
                    solutions=["Follow MSYS2 installation guide"]
                ),
                TroubleshootingStep(
                    title="Update MSYS2",
                    description="Update MSYS2 packages",
                    commands=["pacman -Syu"],
                    solutions=["Restart MSYS2 terminal after updates"]
                )
            ]
        )
    
    def _windows_mingw_section(self) -> TroubleshootingSection:
        return TroubleshootingSection(
            name="MinGW Toolchain",
            description="Install MinGW development tools",
            steps=[
                TroubleshootingStep(
                    title="Install MinGW toolchain",
                    description="Install MinGW-w64 compiler and tools",
                    commands=[
                        "pacman -S mingw-w64-x86_64-toolchain",
                        "pacman -S mingw-w64-x86_64-autotools"
                    ]
                ),
                TroubleshootingStep(
                    title="Verify compiler",
                    description="Check that MinGW compiler is available",
                    commands=["gcc --version", "g++ --version"],
                    expected_output="MinGW compiler version information"
                )
            ]
        )
    
    def _windows_dependencies_section(self) -> TroubleshootingSection:
        return TroubleshootingSection(
            name="Windows Dependencies",
            description="Install BabaChain dependencies on Windows",
            steps=[
                TroubleshootingStep(
                    title="Install build tools",
                    description="Install required build tools in MSYS2",
                    commands=[
                        "pacman -S mingw-w64-x86_64-pkg-config",
                        "pacman -S mingw-w64-x86_64-python3",
                        "pacman -S mingw-w64-x86_64-curl"
                    ]
                )
            ]
        )
    
    def _windows_build_section(self) -> TroubleshootingSection:
        return TroubleshootingSection(
            name="Building on Windows",
            description="Build BabaChain on Windows using MSYS2",
            steps=[
                TroubleshootingStep(
                    title="Set environment",
                    description="Set up build environment variables",
                    commands=[
                        "export PATH=/mingw64/bin:$PATH",
                        "export CC=x86_64-w64-mingw32-gcc",
                        "export CXX=x86_64-w64-mingw32-g++"
                    ]
                ),
                TroubleshootingStep(
                    title="Build dependencies and BabaChain",
                    description="Build using the standard process",
                    commands=[
                        "cd depends && make HOST=x86_64-w64-mingw32 -j4",
                        "./autogen.sh",
                        "CONFIG_SITE=$PWD/depends/x86_64-w64-mingw32/share/config.site ./configure --prefix=/",
                        "make -j4"
                    ]
                )
            ]
        )
    
    def _windows_common_errors_section(self) -> TroubleshootingSection:
        return TroubleshootingSection(
            name="Common Windows Build Errors",
            description="Solutions for common Windows build issues",
            steps=[
                TroubleshootingStep(
                    title="Fix path issues",
                    description="Handle Windows path problems",
                    commands=["echo $PATH", "which gcc"],
                    solutions=["Ensure MinGW64 bin directory is in PATH"]
                ),
                TroubleshootingStep(
                    title="Fix DLL issues",
                    description="Handle missing DLL problems",
                    commands=["ldd <executable>"],
                    solutions=["Copy required DLLs to executable directory"]
                )
            ]
        )
    
    # Generic sections
    def _generic_system_section(self) -> TroubleshootingSection:
        return TroubleshootingSection(
            name="System Requirements",
            description="Check basic system requirements",
            steps=[
                TroubleshootingStep(
                    title="Check system information",
                    description="Gather basic system information",
                    commands=["uname -a", "python3 --version"]
                )
            ]
        )
    
    def _generic_build_tools_section(self) -> TroubleshootingSection:
        return TroubleshootingSection(
            name="Build Tools",
            description="Verify build tools availability",
            steps=[
                TroubleshootingStep(
                    title="Check compilers",
                    description="Verify C/C++ compilers are available",
                    commands=["gcc --version", "g++ --version", "clang --version"]
                )
            ]
        )
    
    def _generic_dependencies_section(self) -> TroubleshootingSection:
        return TroubleshootingSection(
            name="Dependencies",
            description="Check dependency availability",
            steps=[
                TroubleshootingStep(
                    title="Check autotools",
                    description="Verify autotools are installed",
                    commands=["autoconf --version", "automake --version", "libtool --version"]
                )
            ]
        )
    
    def _generic_build_section(self) -> TroubleshootingSection:
        return TroubleshootingSection(
            name="Build Process",
            description="Standard build process",
            steps=[
                TroubleshootingStep(
                    title="Build dependencies",
                    description="Build project dependencies",
                    commands=["cd depends", "make -j4"]
                ),
                TroubleshootingStep(
                    title="Configure and build",
                    description="Configure and compile the project",
                    commands=["./autogen.sh", "./configure", "make -j4"]
                )
            ]
        )

def main():
    if len(sys.argv) > 1 and sys.argv[1] in ['-h', '--help']:
        print("BabaChain Platform-Specific Troubleshooting Guide")
        print("Usage: troubleshooting-guide.py [platform] [format]")
        print("Platforms: macos, linux, windows, generic")
        print("Formats: json, markdown, text, interactive")
        sys.exit(0)
    
    platform = sys.argv[1] if len(sys.argv) > 1 else None
    output_format = sys.argv[2] if len(sys.argv) > 2 else "text"
    
    generator = TroubleshootingGuideGenerator()
    guide = generator.generate_guide(platform)
    
    if output_format == "json":
        print(json.dumps(asdict(guide), indent=2, default=str))
    
    elif output_format == "markdown":
        print(f"# {guide.platform} Troubleshooting Guide\n")
        print(f"**Generated:** {guide.timestamp}")
        print(f"**Platform Version:** {guide.version}\n")
        
        for section in guide.sections:
            print(f"## {section.name}\n")
            print(f"{section.description}\n")
            
            if section.prerequisites:
                print("**Prerequisites:**")
                for prereq in section.prerequisites:
                    print(f"- {prereq}")
                print()
            
            for i, step in enumerate(section.steps, 1):
                print(f"### {i}. {step.title}\n")
                print(f"{step.description}\n")
                
                if step.commands:
                    print("**Commands:**")
                    for cmd in step.commands:
                        print(f"```bash\n{cmd}\n```")
                    print()
                
                if step.expected_output:
                    print(f"**Expected Output:** {step.expected_output}\n")
                
                if step.common_issues:
                    print("**Common Issues:**")
                    for issue in step.common_issues:
                        print(f"- {issue}")
                    print()
                
                if step.solutions:
                    print("**Solutions:**")
                    for solution in step.solutions:
                        print(f"- {solution}")
                    print()
        
        if guide.common_issues:
            print("## Common Issues Quick Reference\n")
            for issue, solution in guide.common_issues.items():
                print(f"**{issue}:** {solution}\n")
        
        if guide.useful_links:
            print("## Useful Links\n")
            for name, url in guide.useful_links.items():
                print(f"- [{name}]({url})")
    
    elif output_format == "interactive":
        print(f"🔧 {guide.platform} Troubleshooting Guide")
        print("=" * 50)
        
        while True:
            print("\nAvailable sections:")
            for i, section in enumerate(guide.sections, 1):
                print(f"{i}. {section.name}")
            print("0. Exit")
            
            try:
                choice = int(input("\nSelect a section (0 to exit): "))
                if choice == 0:
                    break
                elif 1 <= choice <= len(guide.sections):
                    section = guide.sections[choice - 1]
                    print(f"\n📋 {section.name}")
                    print("-" * 40)
                    print(section.description)
                    
                    for i, step in enumerate(section.steps, 1):
                        print(f"\n{i}. {step.title}")
                        print(f"   {step.description}")
                        
                        if step.commands:
                            print("   Commands:")
                            for cmd in step.commands:
                                print(f"   $ {cmd}")
                        
                        input("   Press Enter to continue...")
                else:
                    print("Invalid selection")
            except (ValueError, KeyboardInterrupt):
                break
    
    else:  # text format
        print(f"{guide.platform} Troubleshooting Guide")
        print("=" * 50)
        print(f"Generated: {guide.timestamp}")
        print(f"Platform Version: {guide.version}")
        print()
        
        for section in guide.sections:
            print(f"{section.name}")
            print("-" * len(section.name))
            print(section.description)
            print()
            
            for i, step in enumerate(section.steps, 1):
                print(f"{i}. {step.title}")
                print(f"   {step.description}")
                
                if step.commands:
                    print("   Commands:")
                    for cmd in step.commands:
                        print(f"   $ {cmd}")
                
                if step.common_issues:
                    print("   Common Issues:")
                    for issue in step.common_issues:
                        print(f"   - {issue}")
                
                if step.solutions:
                    print("   Solutions:")
                    for solution in step.solutions:
                        print(f"   - {solution}")
                print()
        
        if guide.common_issues:
            print("Common Issues Quick Reference")
            print("-" * 30)
            for issue, solution in guide.common_issues.items():
                print(f"• {issue}: {solution}")
            print()

if __name__ == "__main__":
    main()