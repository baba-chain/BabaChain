#!/usr/bin/env python3
"""
BabaChain Auto-Node Discovery System
Automatically discovers and connects to the best available nodes
"""

import socket
import threading
import time
import json
import random
from concurrent.futures import ThreadPoolExecutor

class BabaChainNodeDiscovery:
    def __init__(self):
        self.seed_nodes = [
            "seed.babachain.network:9999",
            "node.babachain.io:9999",
            "dnsseed.babachain.org:9999"
        ]
        self.discovered_nodes = []
        self.active_nodes = []
    
    def test_node_connection(self, node_address):
        """Test if a node is reachable"""
        try:
            host, port = node_address.split(':')
            port = int(port)
            
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            result = sock.connect_ex((host, port))
            sock.close()
            
            if result == 0:
                return True
        except:
            pass
        return False
    
    def discover_nodes(self):
        """Discover available nodes"""
        print("🔍 Discovering BabaChain nodes...")
        
        with ThreadPoolExecutor(max_workers=10) as executor:
            futures = []
            
            for node in self.seed_nodes:
                future = executor.submit(self.test_node_connection, node)
                futures.append((node, future))
            
            for node, future in futures:
                try:
                    if future.result(timeout=10):
                        self.active_nodes.append(node)
                        print(f"✅ Found active node: {node}")
                    else:
                        print(f"❌ Node unreachable: {node}")
                except:
                    print(f"⚠️ Node test timeout: {node}")
        
        print(f"📊 Found {len(self.active_nodes)} active nodes")
        return self.active_nodes
    
    def get_best_nodes(self, count=3):
        """Get the best nodes for connection"""
        if not self.active_nodes:
            self.discover_nodes()
        
        # Randomize to distribute load
        best_nodes = random.sample(
            self.active_nodes, 
            min(count, len(self.active_nodes))
        )
        
        return best_nodes
    
    def generate_node_config(self):
        """Generate node configuration for wallet"""
        best_nodes = self.get_best_nodes()
        
        config_lines = []
        for node in best_nodes:
            config_lines.append(f"addnode={node}")
        
        return "\n".join(config_lines)

if __name__ == "__main__":
    discovery = BabaChainNodeDiscovery()
    config = discovery.generate_node_config()
    
    print("\n📝 Generated node configuration:")
    print(config)
    
    # Save to file
    with open("auto_nodes.conf", "w") as f:
        f.write(config)
    
    print("\n💾 Configuration saved to auto_nodes.conf")
