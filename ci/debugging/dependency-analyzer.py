#!/usr/bin/env python3
"""
BabaChain Dependency Tree Analyzer
Analyzes and visualizes dependency relationships and issues
"""

import os
import sys
import json
import subprocess
import re
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple, Any
from dataclasses import dataclass, asdict
from datetime import datetime

@dataclass
class DependencyNode:
    name: str
    version: str
    source: str
    status: str  # available, missing, outdated, conflict
    dependencies: List[str]
    dependents: List[str]
    build_recipe: Optional[str] = None
    checksum: Optional[str] = None
    size: Optional[int] = None

@dataclass
class DependencyIssue:
    type: str  # missing, conflict, circular, outdated
    severity: str  # critical, high, medium, low
    description: str
    affected_packages: List[str]
    suggestion: str

@dataclass
class DependencyAnalysis:
    timestamp: str
    total_packages: int
    resolved_packages: int
    missing_packages: int
    dependency_tree: Dict[str, DependencyNode]
    issues: List[DependencyIssue]
    build_order: List[str]
    circular_dependencies: List[List[str]]

class DependencyAnalyzer:
    def __init__(self, depends_dir: Path = None):
        self.depends_dir = depends_dir or Path('depends')
        self.packages = {}
        self.dependency_graph = {}
        self.reverse_graph = {}
        
    def analyze(self) -> DependencyAnalysis:
        """Perform comprehensive dependency analysis"""
        if not self.depends_dir.exists():
            return self._create_empty_analysis("Dependencies directory not found")
        
        # Load package definitions
        self._load_packages()
        
        # Build dependency graph
        self._build_dependency_graph()
        
        # Analyze issues
        issues = []
        issues.extend(self._find_missing_dependencies())
        issues.extend(self._find_circular_dependencies())
        issues.extend(self._find_version_conflicts())
        issues.extend(self._find_outdated_packages())
        
        # Calculate build order
        build_order = self._calculate_build_order()
        
        # Find circular dependencies
        circular_deps = self._find_circular_dependency_chains()
        
        return DependencyAnalysis(
            timestamp=datetime.now().isoformat(),
            total_packages=len(self.packages),
            resolved_packages=len([p for p in self.packages.values() if p.status == 'available']),
            missing_packages=len([p for p in self.packages.values() if p.status == 'missing']),
            dependency_tree=self.packages,
            issues=issues,
            build_order=build_order,
            circular_dependencies=circular_deps
        )
    
    def _create_empty_analysis(self, reason: str) -> DependencyAnalysis:
        """Create empty analysis with error"""
        return DependencyAnalysis(
            timestamp=datetime.now().isoformat(),
            total_packages=0,
            resolved_packages=0,
            missing_packages=0,
            dependency_tree={},
            issues=[DependencyIssue(
                type="missing",
                severity="critical",
                description=reason,
                affected_packages=[],
                suggestion="Ensure you are in the BabaChain project root directory"
            )],
            build_order=[],
            circular_dependencies=[]
        )
    
    def _load_packages(self):
        """Load package definitions from depends directory"""
        packages_dir = self.depends_dir / 'packages'
        if not packages_dir.exists():
            return
        
        for package_file in packages_dir.glob('*.mk'):
            try:
                package_info = self._parse_package_file(package_file)
                if package_info:
                    self.packages[package_info.name] = package_info
            except Exception as e:
                print(f"Warning: Could not parse {package_file}: {e}", file=sys.stderr)
    
    def _parse_package_file(self, package_file: Path) -> Optional[DependencyNode]:
        """Parse a package makefile"""
        package_name = package_file.stem
        
        with open(package_file, 'r') as f:
            content = f.read()
        
        # Extract package information using regex
        version_match = re.search(rf'{re.escape(package_name)}_version\s*[:=]\s*(.+)', content)
        version = version_match.group(1).strip() if version_match else 'unknown'
        
        # Extract download URL/source
        download_match = re.search(rf'{re.escape(package_name)}_download_path\s*[:=]\s*(.+)', content)
        source = download_match.group(1).strip() if download_match else 'unknown'
        
        # Extract dependencies
        deps_match = re.search(rf'{re.escape(package_name)}_dependencies\s*[:=]\s*(.+)', content)
        dependencies = []
        if deps_match:
            deps_str = deps_match.group(1).strip()
            dependencies = [dep.strip() for dep in deps_str.split() if dep.strip()]
        
        # Extract checksum
        checksum_match = re.search(rf'{re.escape(package_name)}_sha256_hash\s*[:=]\s*(.+)', content)
        checksum = checksum_match.group(1).strip() if checksum_match else None
        
        # Check if package is built
        built_dir = self.depends_dir / 'built'
        status = 'available' if self._is_package_built(package_name, built_dir) else 'missing'
        
        return DependencyNode(
            name=package_name,
            version=version,
            source=source,
            status=status,
            dependencies=dependencies,
            dependents=[],  # Will be filled later
            build_recipe=str(package_file),
            checksum=checksum
        )
    
    def _is_package_built(self, package_name: str, built_dir: Path) -> bool:
        """Check if package is already built"""
        if not built_dir.exists():
            return False
        
        # Look for package-specific build markers
        for host_dir in built_dir.iterdir():
            if host_dir.is_dir():
                package_marker = host_dir / f'{package_name}.built'
                if package_marker.exists():
                    return True
        
        return False
    
    def _build_dependency_graph(self):
        """Build dependency graph and reverse graph"""
        # Build forward graph
        for package_name, package in self.packages.items():
            self.dependency_graph[package_name] = package.dependencies
        
        # Build reverse graph (dependents)
        for package_name, dependencies in self.dependency_graph.items():
            for dep in dependencies:
                if dep not in self.reverse_graph:
                    self.reverse_graph[dep] = []
                self.reverse_graph[dep].append(package_name)
        
        # Update dependents in package objects
        for package_name, dependents in self.reverse_graph.items():
            if package_name in self.packages:
                self.packages[package_name].dependents = dependents
    
    def _find_missing_dependencies(self) -> List[DependencyIssue]:
        """Find missing dependencies"""
        issues = []
        
        for package_name, package in self.packages.items():
            missing_deps = []
            for dep in package.dependencies:
                if dep not in self.packages:
                    missing_deps.append(dep)
            
            if missing_deps:
                issues.append(DependencyIssue(
                    type="missing",
                    severity="critical",
                    description=f"Package {package_name} has missing dependencies: {', '.join(missing_deps)}",
                    affected_packages=[package_name] + missing_deps,
                    suggestion=f"Add package definitions for: {', '.join(missing_deps)}"
                ))
        
        return issues
    
    def _find_circular_dependencies(self) -> List[DependencyIssue]:
        """Find circular dependency issues"""
        issues = []
        circular_chains = self._find_circular_dependency_chains()
        
        for chain in circular_chains:
            issues.append(DependencyIssue(
                type="circular",
                severity="high",
                description=f"Circular dependency detected: {' -> '.join(chain + [chain[0]])}",
                affected_packages=chain,
                suggestion="Break the circular dependency by removing or restructuring dependencies"
            ))
        
        return issues
    
    def _find_circular_dependency_chains(self) -> List[List[str]]:
        """Find circular dependency chains using DFS"""
        visited = set()
        rec_stack = set()
        cycles = []
        
        def dfs(node: str, path: List[str]) -> bool:
            if node in rec_stack:
                # Found a cycle
                cycle_start = path.index(node)
                cycle = path[cycle_start:]
                cycles.append(cycle)
                return True
            
            if node in visited:
                return False
            
            visited.add(node)
            rec_stack.add(node)
            path.append(node)
            
            for neighbor in self.dependency_graph.get(node, []):
                if neighbor in self.packages:  # Only consider known packages
                    dfs(neighbor, path)
            
            rec_stack.remove(node)
            path.pop()
            return False
        
        for package in self.packages:
            if package not in visited:
                dfs(package, [])
        
        return cycles
    
    def _find_version_conflicts(self) -> List[DependencyIssue]:
        """Find version conflicts (placeholder implementation)"""
        issues = []
        
        # This would need more sophisticated version parsing and comparison
        # For now, just check for packages with 'unknown' versions
        unknown_version_packages = [
            name for name, package in self.packages.items() 
            if package.version == 'unknown'
        ]
        
        if unknown_version_packages:
            issues.append(DependencyIssue(
                type="conflict",
                severity="medium",
                description=f"Packages with unknown versions: {', '.join(unknown_version_packages)}",
                affected_packages=unknown_version_packages,
                suggestion="Specify explicit versions for all packages"
            ))
        
        return issues
    
    def _find_outdated_packages(self) -> List[DependencyIssue]:
        """Find potentially outdated packages (placeholder implementation)"""
        issues = []
        
        # This would need integration with package registries or version databases
        # For now, just identify packages that might need updates based on patterns
        potentially_old = []
        
        for name, package in self.packages.items():
            version = package.version.lower()
            # Simple heuristic: very old version patterns
            if any(old_pattern in version for old_pattern in ['2018', '2019', '1.0', '0.9']):
                potentially_old.append(name)
        
        if potentially_old:
            issues.append(DependencyIssue(
                type="outdated",
                severity="low",
                description=f"Potentially outdated packages: {', '.join(potentially_old)}",
                affected_packages=potentially_old,
                suggestion="Check for newer versions of these packages"
            ))
        
        return issues
    
    def _calculate_build_order(self) -> List[str]:
        """Calculate topological build order"""
        # Kahn's algorithm for topological sorting
        in_degree = {package: 0 for package in self.packages}
        
        # Calculate in-degrees
        for package in self.packages:
            for dep in self.dependency_graph.get(package, []):
                if dep in in_degree:
                    in_degree[dep] += 1
        
        # Find packages with no dependencies
        queue = [package for package, degree in in_degree.items() if degree == 0]
        build_order = []
        
        while queue:
            package = queue.pop(0)
            build_order.append(package)
            
            # Reduce in-degree for dependents
            for dependent in self.reverse_graph.get(package, []):
                if dependent in in_degree:
                    in_degree[dependent] -= 1
                    if in_degree[dependent] == 0:
                        queue.append(dependent)
        
        return build_order
    
    def generate_dot_graph(self, output_file: Path, include_missing: bool = False):
        """Generate Graphviz DOT file for dependency visualization"""
        with open(output_file, 'w') as f:
            f.write("digraph dependencies {\n")
            f.write("  rankdir=TB;\n")
            f.write("  node [shape=box];\n\n")
            
            # Add nodes
            for package_name, package in self.packages.items():
                color = {
                    'available': 'lightgreen',
                    'missing': 'lightcoral',
                    'outdated': 'lightyellow',
                    'conflict': 'orange'
                }.get(package.status, 'lightgray')
                
                f.write(f'  "{package_name}" [fillcolor={color}, style=filled, '
                       f'label="{package_name}\\n{package.version}"];\n')
            
            # Add missing dependencies if requested
            if include_missing:
                missing_deps = set()
                for package in self.packages.values():
                    for dep in package.dependencies:
                        if dep not in self.packages:
                            missing_deps.add(dep)
                
                for dep in missing_deps:
                    f.write(f'  "{dep}" [fillcolor=red, style=filled, '
                           f'label="{dep}\\n(missing)"];\n')
            
            f.write("\n")
            
            # Add edges
            for package_name, dependencies in self.dependency_graph.items():
                for dep in dependencies:
                    if dep in self.packages or include_missing:
                        f.write(f'  "{package_name}" -> "{dep}";\n')
            
            f.write("}\n")
    
    def generate_build_script(self, output_file: Path, host: str = "x86_64-pc-linux-gnu"):
        """Generate build script based on dependency order"""
        analysis = self.analyze()
        
        with open(output_file, 'w') as f:
            f.write("#!/bin/bash\n")
            f.write("# Auto-generated dependency build script\n")
            f.write("# Generated by BabaChain Dependency Analyzer\n\n")
            f.write("set -euo pipefail\n\n")
            
            f.write(f"HOST={host}\n")
            f.write("DEPENDS_DIR=$(cd \"$(dirname \"${BASH_SOURCE[0]}\")\" && pwd)\n")
            f.write("cd \"$DEPENDS_DIR\"\n\n")
            
            if analysis.circular_dependencies:
                f.write("# WARNING: Circular dependencies detected!\n")
                for cycle in analysis.circular_dependencies:
                    f.write(f"# Cycle: {' -> '.join(cycle + [cycle[0]])}\n")
                f.write("\n")
            
            f.write("echo \"Building dependencies in dependency order...\"\n\n")
            
            for package in analysis.build_order:
                if package in self.packages:
                    f.write(f"echo \"Building {package}...\"\n")
                    f.write(f"make {package} HOST=$HOST -j$(nproc) || {{\n")
                    f.write(f"  echo \"Failed to build {package}\"\n")
                    f.write(f"  exit 1\n")
                    f.write(f"}}\n\n")
            
            f.write("echo \"All dependencies built successfully!\"\n")
        
        # Make script executable
        os.chmod(output_file, 0o755)

def main():
    if len(sys.argv) > 1 and sys.argv[1] in ['-h', '--help']:
        print("BabaChain Dependency Tree Analyzer")
        print("Usage: dependency-analyzer.py [command] [options]")
        print("Commands:")
        print("  analyze [format]     - Analyze dependencies (json, markdown, text)")
        print("  graph <output.dot>   - Generate Graphviz DOT file")
        print("  build-script <file>  - Generate build script")
        print("  tree [package]       - Show dependency tree")
        sys.exit(0)
    
    command = sys.argv[1] if len(sys.argv) > 1 else "analyze"
    
    analyzer = DependencyAnalyzer()
    
    if command == "analyze":
        output_format = sys.argv[2] if len(sys.argv) > 2 else "text"
        analysis = analyzer.analyze()
        
        if output_format == "json":
            print(json.dumps(asdict(analysis), indent=2, default=str))
        
        elif output_format == "markdown":
            print("# Dependency Analysis Report\n")
            print(f"**Timestamp:** {analysis.timestamp}")
            print(f"**Total Packages:** {analysis.total_packages}")
            print(f"**Resolved:** {analysis.resolved_packages}")
            print(f"**Missing:** {analysis.missing_packages}\n")
            
            if analysis.issues:
                print("## Issues\n")
                for issue in analysis.issues:
                    severity_icon = {"critical": "🔴", "high": "🟠", "medium": "🟡", "low": "🔵"}
                    icon = severity_icon.get(issue.severity, "⚪")
                    print(f"### {icon} {issue.type.title()}: {issue.description}")
                    print(f"**Affected packages:** {', '.join(issue.affected_packages)}")
                    print(f"**Suggestion:** {issue.suggestion}\n")
            
            if analysis.build_order:
                print("## Build Order\n")
                for i, package in enumerate(analysis.build_order, 1):
                    print(f"{i}. {package}")
                print()
            
            if analysis.circular_dependencies:
                print("## Circular Dependencies\n")
                for cycle in analysis.circular_dependencies:
                    print(f"- {' → '.join(cycle + [cycle[0]])}")
                print()
        
        else:  # text format
            print("Dependency Analysis Report")
            print("=" * 40)
            print(f"Timestamp: {analysis.timestamp}")
            print(f"Total packages: {analysis.total_packages}")
            print(f"Resolved: {analysis.resolved_packages}")
            print(f"Missing: {analysis.missing_packages}")
            print()
            
            if analysis.issues:
                print("Issues:")
                for issue in analysis.issues:
                    print(f"  [{issue.severity.upper()}] {issue.type}: {issue.description}")
                    print(f"    Suggestion: {issue.suggestion}")
                print()
            
            if analysis.build_order:
                print("Build order:")
                for i, package in enumerate(analysis.build_order, 1):
                    status = analysis.dependency_tree.get(package, DependencyNode("", "", "", "unknown", [], [])).status
                    status_symbol = {"available": "✓", "missing": "✗", "outdated": "⚠"}.get(status, "?")
                    print(f"  {i:2d}. [{status_symbol}] {package}")
                print()
            
            if analysis.circular_dependencies:
                print("Circular dependencies:")
                for cycle in analysis.circular_dependencies:
                    print(f"  {' -> '.join(cycle + [cycle[0]])}")
                print()
    
    elif command == "graph":
        if len(sys.argv) < 3:
            print("Usage: dependency-analyzer.py graph <output.dot>")
            sys.exit(1)
        
        output_file = Path(sys.argv[2])
        include_missing = "--include-missing" in sys.argv
        
        analyzer.generate_dot_graph(output_file, include_missing)
        print(f"Dependency graph saved to {output_file}")
        print("To generate PNG: dot -Tpng output.dot -o output.png")
    
    elif command == "build-script":
        if len(sys.argv) < 3:
            print("Usage: dependency-analyzer.py build-script <output.sh>")
            sys.exit(1)
        
        output_file = Path(sys.argv[2])
        host = sys.argv[3] if len(sys.argv) > 3 else "x86_64-pc-linux-gnu"
        
        analyzer.generate_build_script(output_file, host)
        print(f"Build script saved to {output_file}")
    
    elif command == "tree":
        package_name = sys.argv[2] if len(sys.argv) > 2 else None
        analysis = analyzer.analyze()
        
        def print_tree(pkg_name: str, level: int = 0, visited: Set[str] = None):
            if visited is None:
                visited = set()
            
            if pkg_name in visited:
                print("  " * level + f"{pkg_name} (circular)")
                return
            
            visited.add(pkg_name)
            
            if pkg_name in analysis.dependency_tree:
                package = analysis.dependency_tree[pkg_name]
                status_symbol = {"available": "✓", "missing": "✗", "outdated": "⚠"}.get(package.status, "?")
                print("  " * level + f"[{status_symbol}] {pkg_name} ({package.version})")
                
                for dep in package.dependencies:
                    print_tree(dep, level + 1, visited.copy())
            else:
                print("  " * level + f"[✗] {pkg_name} (missing)")
        
        if package_name:
            print(f"Dependency tree for {package_name}:")
            print_tree(package_name)
        else:
            print("All packages:")
            for pkg in analysis.build_order:
                print_tree(pkg)
                print()
    
    else:
        print(f"Unknown command: {command}")
        sys.exit(1)

if __name__ == "__main__":
    main()