#!/usr/bin/env python3
"""
BabaChain Build Performance Monitor

This script monitors build performance, tracks metrics, and detects regressions.
"""

import os
import sys
import json
import time
import psutil
import subprocess
import threading
import argparse
import logging
from datetime import datetime, timedelta
from pathlib import Path
import statistics

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class BuildMonitor:
    def __init__(self, output_dir=None):
        self.output_dir = Path(output_dir) if output_dir else Path.cwd() / 'build-metrics'
        self.output_dir.mkdir(exist_ok=True)
        
        self.monitoring = False
        self.start_time = None
        self.metrics = {
            'cpu_usage': [],
            'memory_usage': [],
            'disk_io': [],
            'network_io': [],
            'build_phases': [],
            'timestamps': []
        }
        
        # Performance thresholds
        self.thresholds = {
            'max_memory_gb': 8.0,
            'max_cpu_percent': 90.0,
            'max_build_time_minutes': 60.0,
            'warning_memory_gb': 6.0,
            'warning_cpu_percent': 80.0
        }
    
    def start_monitoring(self):
        """Start system resource monitoring"""
        self.monitoring = True
        self.start_time = time.time()
        
        logger.info("Starting build performance monitoring...")
        
        # Start monitoring thread
        self.monitor_thread = threading.Thread(target=self._monitor_resources)
        self.monitor_thread.daemon = True
        self.monitor_thread.start()
    
    def stop_monitoring(self):
        """Stop system resource monitoring"""
        self.monitoring = False
        if hasattr(self, 'monitor_thread'):
            self.monitor_thread.join(timeout=5)
        
        logger.info("Stopped build performance monitoring")
    
    def _monitor_resources(self):
        """Monitor system resources in background thread"""
        initial_disk_io = psutil.disk_io_counters()
        initial_net_io = psutil.net_io_counters()
        
        while self.monitoring:
            try:
                timestamp = time.time()
                
                # CPU usage
                cpu_percent = psutil.cpu_percent(interval=1)
                
                # Memory usage
                memory = psutil.virtual_memory()
                memory_gb = memory.used / (1024**3)
                
                # Disk I/O
                disk_io = psutil.disk_io_counters()
                disk_read_mb = (disk_io.read_bytes - initial_disk_io.read_bytes) / (1024**2)
                disk_write_mb = (disk_io.write_bytes - initial_disk_io.write_bytes) / (1024**2)
                
                # Network I/O
                net_io = psutil.net_io_counters()
                net_sent_mb = (net_io.bytes_sent - initial_net_io.bytes_sent) / (1024**2)
                net_recv_mb = (net_io.bytes_recv - initial_net_io.bytes_recv) / (1024**2)
                
                # Store metrics
                self.metrics['timestamps'].append(timestamp)
                self.metrics['cpu_usage'].append(cpu_percent)
                self.metrics['memory_usage'].append(memory_gb)
                self.metrics['disk_io'].append({
                    'read_mb': disk_read_mb,
                    'write_mb': disk_write_mb
                })
                self.metrics['network_io'].append({
                    'sent_mb': net_sent_mb,
                    'recv_mb': net_recv_mb
                })
                
                # Check thresholds
                if memory_gb > self.thresholds['warning_memory_gb']:
                    logger.warning(f"High memory usage: {memory_gb:.1f}GB")
                
                if cpu_percent > self.thresholds['warning_cpu_percent']:
                    logger.warning(f"High CPU usage: {cpu_percent:.1f}%")
                
                time.sleep(5)  # Monitor every 5 seconds
                
            except Exception as e:
                logger.error(f"Error monitoring resources: {e}")
                time.sleep(5)
    
    def log_build_phase(self, phase_name, start_time=None):
        """Log a build phase with timing"""
        current_time = time.time()
        
        if start_time:
            duration = current_time - start_time
            self.metrics['build_phases'].append({
                'phase': phase_name,
                'start_time': start_time,
                'end_time': current_time,
                'duration_seconds': duration
            })
            logger.info(f"Build phase '{phase_name}' completed in {duration:.1f} seconds")
        else:
            logger.info(f"Build phase '{phase_name}' started")
            return current_time
    
    def generate_report(self, build_info=None):
        """Generate comprehensive build performance report"""
        if not self.start_time:
            logger.error("No monitoring data available")
            return None
        
        end_time = time.time()
        total_duration = end_time - self.start_time
        
        # Calculate statistics
        report = {
            'timestamp': datetime.now().isoformat(),
            'build_info': build_info or {},
            'duration': {
                'total_seconds': total_duration,
                'total_minutes': total_duration / 60,
                'start_time': datetime.fromtimestamp(self.start_time).isoformat(),
                'end_time': datetime.fromtimestamp(end_time).isoformat()
            },
            'resource_usage': {
                'cpu': {
                    'max_percent': max(self.metrics['cpu_usage']) if self.metrics['cpu_usage'] else 0,
                    'avg_percent': statistics.mean(self.metrics['cpu_usage']) if self.metrics['cpu_usage'] else 0,
                    'samples': len(self.metrics['cpu_usage'])
                },
                'memory': {
                    'max_gb': max(self.metrics['memory_usage']) if self.metrics['memory_usage'] else 0,
                    'avg_gb': statistics.mean(self.metrics['memory_usage']) if self.metrics['memory_usage'] else 0,
                    'samples': len(self.metrics['memory_usage'])
                },
                'disk_io': {
                    'total_read_mb': sum(io['read_mb'] for io in self.metrics['disk_io']),
                    'total_write_mb': sum(io['write_mb'] for io in self.metrics['disk_io']),
                    'samples': len(self.metrics['disk_io'])
                },
                'network_io': {
                    'total_sent_mb': sum(io['sent_mb'] for io in self.metrics['network_io']),
                    'total_recv_mb': sum(io['recv_mb'] for io in self.metrics['network_io']),
                    'samples': len(self.metrics['network_io'])
                }
            },
            'build_phases': self.metrics['build_phases'],
            'performance_analysis': self._analyze_performance(total_duration),
            'recommendations': self._generate_recommendations()
        }
        
        # Save detailed report
        report_file = self.output_dir / f'build-report-{datetime.now().strftime("%Y%m%d-%H%M%S")}.json'
        with open(report_file, 'w') as f:
            json.dump(report, f, indent=2)
        
        # Generate human-readable summary
        self._generate_summary_report(report)
        
        logger.info(f"Build performance report saved to {report_file}")
        return report
    
    def _analyze_performance(self, total_duration):
        """Analyze build performance and identify issues"""
        analysis = {
            'overall_rating': 'good',
            'issues': [],
            'strengths': []
        }
        
        # Analyze build time
        if total_duration > self.thresholds['max_build_time_minutes'] * 60:
            analysis['overall_rating'] = 'poor'
            analysis['issues'].append(f"Build time exceeded threshold: {total_duration/60:.1f} minutes")
        elif total_duration > (self.thresholds['max_build_time_minutes'] * 60 * 0.8):
            analysis['overall_rating'] = 'fair'
            analysis['issues'].append(f"Build time approaching threshold: {total_duration/60:.1f} minutes")
        else:
            analysis['strengths'].append(f"Good build time: {total_duration/60:.1f} minutes")
        
        # Analyze memory usage
        if self.metrics['memory_usage']:
            max_memory = max(self.metrics['memory_usage'])
            if max_memory > self.thresholds['max_memory_gb']:
                analysis['overall_rating'] = 'poor'
                analysis['issues'].append(f"Memory usage exceeded threshold: {max_memory:.1f}GB")
            elif max_memory > self.thresholds['warning_memory_gb']:
                if analysis['overall_rating'] == 'good':
                    analysis['overall_rating'] = 'fair'
                analysis['issues'].append(f"High memory usage: {max_memory:.1f}GB")
            else:
                analysis['strengths'].append(f"Efficient memory usage: {max_memory:.1f}GB peak")
        
        # Analyze CPU usage
        if self.metrics['cpu_usage']:
            max_cpu = max(self.metrics['cpu_usage'])
            avg_cpu = statistics.mean(self.metrics['cpu_usage'])
            
            if max_cpu > self.thresholds['max_cpu_percent']:
                analysis['issues'].append(f"CPU usage peaked at {max_cpu:.1f}%")
            
            if avg_cpu < 50:
                analysis['issues'].append(f"Low average CPU utilization: {avg_cpu:.1f}% (may indicate I/O bottleneck)")
            else:
                analysis['strengths'].append(f"Good CPU utilization: {avg_cpu:.1f}% average")
        
        return analysis
    
    def _generate_recommendations(self):
        """Generate performance improvement recommendations"""
        recommendations = []
        
        if not self.metrics['memory_usage'] or not self.metrics['cpu_usage']:
            return ["Unable to generate recommendations - insufficient monitoring data"]
        
        max_memory = max(self.metrics['memory_usage'])
        avg_cpu = statistics.mean(self.metrics['cpu_usage'])
        
        # Memory recommendations
        if max_memory > self.thresholds['warning_memory_gb']:
            recommendations.append("Consider reducing parallel build jobs to lower memory usage")
            recommendations.append("Enable ccache to reduce compilation memory requirements")
        
        # CPU recommendations
        if avg_cpu < 50:
            recommendations.append("CPU utilization is low - consider increasing parallel build jobs")
            recommendations.append("Check for I/O bottlenecks (slow disk or network)")
        elif avg_cpu > 85:
            recommendations.append("High CPU usage - system may be overloaded")
        
        # Build phase recommendations
        if self.metrics['build_phases']:
            longest_phase = max(self.metrics['build_phases'], key=lambda x: x['duration_seconds'])
            if longest_phase['duration_seconds'] > 300:  # 5 minutes
                recommendations.append(f"Optimize '{longest_phase['phase']}' phase - took {longest_phase['duration_seconds']/60:.1f} minutes")
        
        # Disk I/O recommendations
        if self.metrics['disk_io']:
            total_disk_io = sum(io['read_mb'] + io['write_mb'] for io in self.metrics['disk_io'])
            if total_disk_io > 1000:  # 1GB
                recommendations.append("High disk I/O detected - consider using SSD or faster storage")
                recommendations.append("Enable build caching to reduce disk I/O")
        
        return recommendations if recommendations else ["Build performance is optimal - no specific recommendations"]
    
    def _generate_summary_report(self, report):
        """Generate human-readable summary report"""
        summary_file = self.output_dir / 'latest-build-summary.md'
        
        with open(summary_file, 'w') as f:
            f.write(f"# Build Performance Summary\n\n")
            f.write(f"**Generated:** {report['timestamp']}\n")
            f.write(f"**Build Duration:** {report['duration']['total_minutes']:.1f} minutes\n")
            f.write(f"**Performance Rating:** {report['performance_analysis']['overall_rating'].upper()}\n\n")
            
            f.write(f"## Resource Usage\n\n")
            f.write(f"- **CPU:** {report['resource_usage']['cpu']['max_percent']:.1f}% peak, {report['resource_usage']['cpu']['avg_percent']:.1f}% average\n")
            f.write(f"- **Memory:** {report['resource_usage']['memory']['max_gb']:.1f}GB peak, {report['resource_usage']['memory']['avg_gb']:.1f}GB average\n")
            f.write(f"- **Disk I/O:** {report['resource_usage']['disk_io']['total_read_mb']:.1f}MB read, {report['resource_usage']['disk_io']['total_write_mb']:.1f}MB write\n")
            f.write(f"- **Network I/O:** {report['resource_usage']['network_io']['total_sent_mb']:.1f}MB sent, {report['resource_usage']['network_io']['total_recv_mb']:.1f}MB received\n\n")
            
            if report['build_phases']:
                f.write(f"## Build Phases\n\n")
                for phase in report['build_phases']:
                    f.write(f"- **{phase['phase']}:** {phase['duration_seconds']:.1f} seconds\n")
                f.write(f"\n")
            
            if report['performance_analysis']['issues']:
                f.write(f"## ⚠️ Performance Issues\n\n")
                for issue in report['performance_analysis']['issues']:
                    f.write(f"- {issue}\n")
                f.write(f"\n")
            
            if report['performance_analysis']['strengths']:
                f.write(f"## ✅ Performance Strengths\n\n")
                for strength in report['performance_analysis']['strengths']:
                    f.write(f"- {strength}\n")
                f.write(f"\n")
            
            f.write(f"## 💡 Recommendations\n\n")
            for rec in report['recommendations']:
                f.write(f"- {rec}\n")
    
    def compare_with_baseline(self, baseline_file):
        """Compare current metrics with baseline performance"""
        try:
            with open(baseline_file, 'r') as f:
                baseline = json.load(f)
            
            if not self.metrics['memory_usage'] or not self.metrics['cpu_usage']:
                logger.error("No current metrics to compare")
                return None
            
            current_duration = time.time() - self.start_time if self.start_time else 0
            baseline_duration = baseline.get('duration', {}).get('total_seconds', 0)
            
            comparison = {
                'timestamp': datetime.now().isoformat(),
                'baseline_file': str(baseline_file),
                'duration_change_percent': ((current_duration - baseline_duration) / baseline_duration * 100) if baseline_duration > 0 else 0,
                'memory_change_percent': 0,
                'cpu_change_percent': 0,
                'regression_detected': False
            }
            
            # Compare memory usage
            current_max_memory = max(self.metrics['memory_usage'])
            baseline_max_memory = baseline.get('resource_usage', {}).get('memory', {}).get('max_gb', 0)
            if baseline_max_memory > 0:
                comparison['memory_change_percent'] = (current_max_memory - baseline_max_memory) / baseline_max_memory * 100
            
            # Compare CPU usage
            current_avg_cpu = statistics.mean(self.metrics['cpu_usage'])
            baseline_avg_cpu = baseline.get('resource_usage', {}).get('cpu', {}).get('avg_percent', 0)
            if baseline_avg_cpu > 0:
                comparison['cpu_change_percent'] = (current_avg_cpu - baseline_avg_cpu) / baseline_avg_cpu * 100
            
            # Detect regressions
            if (comparison['duration_change_percent'] > 20 or 
                comparison['memory_change_percent'] > 25 or
                comparison['cpu_change_percent'] > 30):
                comparison['regression_detected'] = True
                logger.warning("Performance regression detected!")
            
            # Save comparison report
            comparison_file = self.output_dir / f'performance-comparison-{datetime.now().strftime("%Y%m%d-%H%M%S")}.json'
            with open(comparison_file, 'w') as f:
                json.dump(comparison, f, indent=2)
            
            logger.info(f"Performance comparison saved to {comparison_file}")
            return comparison
            
        except Exception as e:
            logger.error(f"Failed to compare with baseline: {e}")
            return None

def main():
    parser = argparse.ArgumentParser(description='BabaChain Build Performance Monitor')
    parser.add_argument('--output-dir', help='Output directory for reports')
    parser.add_argument('--baseline', help='Baseline performance file for comparison')
    parser.add_argument('--duration', type=int, default=3600, help='Maximum monitoring duration in seconds')
    parser.add_argument('--build-command', help='Build command to execute and monitor')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose logging')
    
    args = parser.parse_args()
    
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    
    monitor = BuildMonitor(args.output_dir)
    
    try:
        monitor.start_monitoring()
        
        if args.build_command:
            logger.info(f"Executing build command: {args.build_command}")
            phase_start = monitor.log_build_phase("build_execution")
            
            result = subprocess.run(args.build_command, shell=True, capture_output=True, text=True)
            
            monitor.log_build_phase("build_execution", phase_start)
            
            if result.returncode != 0:
                logger.error(f"Build command failed with exit code {result.returncode}")
                logger.error(f"Error output: {result.stderr}")
            else:
                logger.info("Build command completed successfully")
        else:
            logger.info(f"Monitoring for {args.duration} seconds...")
            time.sleep(args.duration)
        
        monitor.stop_monitoring()
        
        # Generate report
        build_info = {
            'command': args.build_command,
            'exit_code': result.returncode if args.build_command else None
        }
        
        report = monitor.generate_report(build_info)
        
        # Compare with baseline if provided
        if args.baseline and Path(args.baseline).exists():
            comparison = monitor.compare_with_baseline(args.baseline)
            if comparison and comparison['regression_detected']:
                logger.error("Performance regression detected!")
                sys.exit(1)
        
        logger.info("Build monitoring completed successfully")
        
    except KeyboardInterrupt:
        logger.info("Monitoring interrupted by user")
        monitor.stop_monitoring()
    except Exception as e:
        logger.error(f"Build monitoring failed: {e}")
        monitor.stop_monitoring()
        sys.exit(1)

if __name__ == '__main__':
    main()