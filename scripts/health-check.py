#!/usr/bin/env python3
# BabaChain Mainnet Health Check Script

import subprocess
import json
import time
import requests
from datetime import datetime

class BabaChainHealthMonitor:
    def __init__(self):
        self.rpc_user = "babachain_mainnet"
        self.rpc_password = "CHANGE_THIS_PASSWORD"
        self.rpc_port = 9998
        
    def check_node_status(self):
        try:
            result = subprocess.run([
                "babachain-cli",
                f"-rpcuser={self.rpc_user}",
                f"-rpcpassword={self.rpc_password}",
                f"-rpcport={self.rpc_port}",
                "getnetworkinfo"
            ], capture_output=True, text=True)
            
            if result.returncode == 0:
                return json.loads(result.stdout)
            return None
        except:
            return None
    
    def check_staking_status(self):
        try:
            result = subprocess.run([
                "babachain-cli", 
                f"-rpcuser={self.rpc_user}",
                f"-rpcpassword={self.rpc_password}",
                f"-rpcport={self.rpc_port}",
                "getstakinginfo"
            ], capture_output=True, text=True)
            
            if result.returncode == 0:
                return json.loads(result.stdout)
            return None
        except:
            return None
    
    def run_health_check(self):
        print(f"🏥 BabaChain Health Check - {datetime.now().isoformat()}")
        print("=" * 60)
        
        # Check node status
        network_info = self.check_node_status()
        if network_info:
            print(f"✅ Node Status: Online")
            print(f"   Version: {network_info.get('version', 'Unknown')}")
            print(f"   Connections: {network_info.get('connections', 0)}")
            print(f"   Network Active: {network_info.get('networkactive', False)}")
        else:
            print("❌ Node Status: Offline or unreachable")
        
        # Check staking status
        staking_info = self.check_staking_status()
        if staking_info:
            print(f"✅ Staking Status: {staking_info.get('enabled', False)}")
            print(f"   Staking: {staking_info.get('staking', False)}")
            print(f"   Expected Time: {staking_info.get('expectedtime', 'Unknown')}")
        else:
            print("❌ Staking Status: Unable to retrieve")
        
        print("=" * 60)

if __name__ == "__main__":
    monitor = BabaChainHealthMonitor()
    monitor.run_health_check()
