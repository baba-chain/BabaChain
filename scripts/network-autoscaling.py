#!/usr/bin/env python3
"""
BabaChain Network Auto-Scaling and Load Balancing System
Automatically manages network capacity and node distribution
"""

import json
import time
import threading
import subprocess
import socket
import random
from datetime import datetime, timedelta
from concurrent.futures import ThreadPoolExecutor
import logging

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class BabaChainNetworkManager:
    def __init__(self, config_file="config/wallet-integration.json"):
        self.config = self.load_config(config_file)
        self.active_nodes = []
        self.node_stats = {}
        self.network_health = {}
        self.scaling_metrics = {
            'total_connections': 0,
            'avg_response_time': 0,
            'network_load': 0,
            'staking_nodes': 0
        }
        self.running = False
    
    def load_config(self, config_file):
        """Load configuration from JSON file"""
        try:
            with open(config_file, 'r') as f:
                return json.load(f)
        except FileNotFoundError:
            logger.error(f"Configuration file {config_file} not found")
            return self.get_default_config()
    
    def get_default_config(self):
        """Return default configuration"""
        return {
            "babachain_integration": {
                "network": {
                    "mainnet": {
                        "seed_nodes": [
                            "dnsseed.babachain.org:9999",
                            "seed.babachain.network:9999",
                            "node.babachain.io:9999"
                        ]
                    }
                },
                "auto_features": {
                    "node_discovery": {
                        "enabled": True,
                        "discovery_interval": 300,
                        "max_peers": 125
                    },
                    "network_healing": {
                        "enabled": True,
                        "reconnect_interval": 60
                    }
                }
            }
        }
    
    def discover_network_nodes(self):
        """Discover active nodes in the network"""
        logger.info("🔍 Starting network node discovery...")
        
        seed_nodes = self.config["babachain_integration"]["network"]["mainnet"]["seed_nodes"]
        discovered_nodes = []
        
        with ThreadPoolExecutor(max_workers=20) as executor:
            futures = []
            
            for seed_node in seed_nodes:
                future = executor.submit(self.test_node_connectivity, seed_node)
                futures.append((seed_node, future))
            
            for node, future in futures:
                try:
                    if future.result(timeout=10):
                        discovered_nodes.append(node)
                        logger.info(f"✅ Active node discovered: {node}")
                    else:
                        logger.warning(f"❌ Node unreachable: {node}")
                except Exception as e:
                    logger.error(f"⚠️ Error testing node {node}: {e}")
        
        self.active_nodes = discovered_nodes
        logger.info(f"📊 Discovery complete: {len(discovered_nodes)} active nodes found")
        return discovered_nodes
    
    def test_node_connectivity(self, node_address):
        """Test connectivity to a specific node"""
        try:
            host, port = node_address.split(':')
            port = int(port)
            
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            
            start_time = time.time()
            result = sock.connect_ex((host, port))
            response_time = time.time() - start_time
            
            sock.close()
            
            if result == 0:
                self.node_stats[node_address] = {
                    'status': 'active',
                    'response_time': response_time,
                    'last_check': datetime.now().isoformat()
                }
                return True
            else:
                self.node_stats[node_address] = {
                    'status': 'inactive',
                    'response_time': None,
                    'last_check': datetime.now().isoformat()
                }
                return False
        except Exception as e:
            logger.error(f"Connection test failed for {node_address}: {e}")
            return False
    
    def monitor_network_health(self):
        """Monitor overall network health"""
        logger.info("🏥 Starting network health monitoring...")
        
        while self.running:
            try:
                # Test active nodes
                active_count = 0
                total_response_time = 0
                
                for node in self.active_nodes:
                    if self.test_node_connectivity(node):
                        active_count += 1
                        if node in self.node_stats:
                            total_response_time += self.node_stats[node].get('response_time', 0)
                
                # Calculate health metrics
                if active_count > 0:
                    avg_response_time = total_response_time / active_count
                    network_health_score = min(100, (active_count / len(self.active_nodes)) * 100)
                else:
                    avg_response_time = float('inf')
                    network_health_score = 0
                
                self.network_health = {
                    'active_nodes': active_count,
                    'total_nodes': len(self.active_nodes),
                    'health_score': network_health_score,
                    'avg_response_time': avg_response_time,
                    'timestamp': datetime.now().isoformat()
                }
                
                logger.info(f"📊 Network Health: {network_health_score:.1f}% ({active_count}/{len(self.active_nodes)} nodes)")
                
                # Trigger scaling if needed
                if network_health_score < 70:
                    self.trigger_network_scaling()
                
                time.sleep(30)  # Check every 30 seconds
                
            except Exception as e:
                logger.error(f"Health monitoring error: {e}")
                time.sleep(60)
    
    def trigger_network_scaling(self):
        """Trigger network scaling when health is poor"""
        logger.warning("⚠️ Network health degraded, triggering scaling...")
        
        # Rediscover nodes
        self.discover_network_nodes()
        
        # Implement load balancing
        self.balance_network_load()
        
        # Notify administrators (in production, this would send alerts)
        logger.info("📧 Network scaling triggered - administrators notified")
    
    def balance_network_load(self):
        """Balance load across available nodes"""
        logger.info("⚖️ Balancing network load...")
        
        if not self.active_nodes:
            logger.error("No active nodes available for load balancing")
            return
        
        # Sort nodes by response time (best first)
        sorted_nodes = sorted(
            self.active_nodes,
            key=lambda node: self.node_stats.get(node, {}).get('response_time', float('inf'))
        )
        
        # Select best nodes for different purposes
        primary_nodes = sorted_nodes[:3]  # Best 3 for primary connections
        backup_nodes = sorted_nodes[3:6]  # Next 3 for backup
        
        # Generate optimized node configuration
        config = self.generate_optimized_config(primary_nodes, backup_nodes)
        
        # Save configuration
        with open('config/optimized_nodes.conf', 'w') as f:
            f.write(config)
        
        logger.info(f"✅ Load balancing complete - {len(primary_nodes)} primary nodes configured")
    
    def generate_optimized_config(self, primary_nodes, backup_nodes):
        """Generate optimized node configuration"""
        config_lines = [
            "# BabaChain Optimized Node Configuration",
            f"# Generated: {datetime.now().isoformat()}",
            "",
            "# Primary nodes (best performance)",
        ]
        
        for node in primary_nodes:
            config_lines.append(f"addnode={node}")
        
        config_lines.extend([
            "",
            "# Backup nodes",
        ])
        
        for node in backup_nodes:
            config_lines.append(f"addnode={node}")
        
        config_lines.extend([
            "",
            "# Auto-scaling settings",
            "maxconnections=125",
            "timeout=5000",
            "",
            "# Performance optimization",
            "dbcache=300",
            "maxmempool=300",
        ])
        
        return "\n".join(config_lines)
    
    def start_auto_scaling(self):
        """Start the auto-scaling system"""
        logger.info("🚀 Starting BabaChain Network Auto-Scaling System...")
        
        self.running = True
        
        # Initial discovery
        self.discover_network_nodes()
        
        # Start monitoring thread
        monitor_thread = threading.Thread(target=self.monitor_network_health)
        monitor_thread.daemon = True
        monitor_thread.start()
        
        # Start periodic discovery
        discovery_thread = threading.Thread(target=self.periodic_discovery)
        discovery_thread.daemon = True
        discovery_thread.start()
        
        logger.info("✅ Auto-scaling system started successfully")
    
    def periodic_discovery(self):
        """Periodically rediscover nodes"""
        discovery_interval = self.config["babachain_integration"]["auto_features"]["node_discovery"]["discovery_interval"]
        
        while self.running:
            time.sleep(discovery_interval)
            logger.info("🔄 Performing periodic node discovery...")
            self.discover_network_nodes()
            self.balance_network_load()
    
    def stop_auto_scaling(self):
        """Stop the auto-scaling system"""
        logger.info("🛑 Stopping auto-scaling system...")
        self.running = False
    
    def get_network_status(self):
        """Get current network status"""
        return {
            'active_nodes': len(self.active_nodes),
            'network_health': self.network_health,
            'node_stats': self.node_stats,
            'scaling_metrics': self.scaling_metrics
        }
    
    def generate_wallet_config(self, wallet_type="desktop"):
        """Generate wallet-specific configuration"""
        if not self.active_nodes:
            self.discover_network_nodes()
        
        # Select best nodes for this wallet type
        best_nodes = self.select_nodes_for_wallet(wallet_type)
        
        config = {
            'network': {
                'nodes': best_nodes,
                'max_connections': self.get_max_connections(wallet_type),
                'timeout': self.get_timeout(wallet_type)
            },
            'staking': {
                'enabled': True,
                'auto_start': True
            }
        }
        
        return config
    
    def select_nodes_for_wallet(self, wallet_type):
        """Select optimal nodes for specific wallet type"""
        if wallet_type == "mobile":
            # Mobile wallets need fewer, faster connections
            return self.get_fastest_nodes(3)
        elif wallet_type == "desktop":
            # Desktop wallets can handle more connections
            return self.get_fastest_nodes(8)
        else:
            return self.get_fastest_nodes(5)
    
    def get_fastest_nodes(self, count):
        """Get the fastest responding nodes"""
        if not self.active_nodes:
            return []
        
        sorted_nodes = sorted(
            self.active_nodes,
            key=lambda node: self.node_stats.get(node, {}).get('response_time', float('inf'))
        )
        
        return sorted_nodes[:count]
    
    def get_max_connections(self, wallet_type):
        """Get max connections for wallet type"""
        limits = {
            'mobile': 8,
            'desktop': 125,
            'server': 200
        }
        return limits.get(wallet_type, 50)
    
    def get_timeout(self, wallet_type):
        """Get timeout for wallet type"""
        timeouts = {
            'mobile': 10000,  # 10 seconds for mobile
            'desktop': 5000,  # 5 seconds for desktop
            'server': 3000    # 3 seconds for servers
        }
        return timeouts.get(wallet_type, 5000)

def main():
    """Main function"""
    print("🌐 BabaChain Network Auto-Scaling System")
    print("=" * 50)
    
    # Create network manager
    network_manager = BabaChainNetworkManager()
    
    try:
        # Start auto-scaling
        network_manager.start_auto_scaling()
        
        # Run for demonstration
        print("Running auto-scaling system... (Press Ctrl+C to stop)")
        
        while True:
            time.sleep(10)
            status = network_manager.get_network_status()
            print(f"\n📊 Network Status:")
            print(f"   Active Nodes: {status['active_nodes']}")
            if status['network_health']:
                print(f"   Health Score: {status['network_health']['health_score']:.1f}%")
                print(f"   Avg Response: {status['network_health']['avg_response_time']:.3f}s")
    
    except KeyboardInterrupt:
        print("\n🛑 Shutting down auto-scaling system...")
        network_manager.stop_auto_scaling()
        print("✅ Shutdown complete")

if __name__ == "__main__":
    main()