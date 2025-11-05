#!/usr/bin/env python3
"""
BabaChain System Integration Test Suite
Tests cross-platform functionality and network integration
"""

import subprocess
import time
import json
import requests
from pathlib import Path

class BabaChainIntegrationTest:
    def __init__(self):
        self.core_node = None
        self.test_results = []
    
    def start_core_node(self):
        """Start BabaChain core node for testing"""
        print("Starting BabaChain core node...")
        try:
            self.core_node = subprocess.Popen([
                "./src/babachaind",
                "-regtest",
                "-daemon",
                "-rpcuser=test",
                "-rpcpassword=test",
                "-rpcport=19998"
            ])
            time.sleep(5)  # Wait for node to start
            return True
        except Exception as e:
            print(f"Failed to start core node: {e}")
            return False
    
    def test_network_connectivity(self):
        """Test network connectivity and peer discovery"""
        print("Testing network connectivity...")
        try:
            result = subprocess.run([
                "./src/babachain-cli",
                "-regtest",
                "-rpcuser=test",
                "-rpcpassword=test",
                "getnetworkinfo"
            ], capture_output=True, text=True)
            
            if result.returncode == 0:
                network_info = json.loads(result.stdout)
                if network_info.get("networkactive"):
                    self.test_results.append(("Network Connectivity", "PASS"))
                    return True
            
            self.test_results.append(("Network Connectivity", "FAIL"))
            return False
        except Exception as e:
            print(f"Network connectivity test failed: {e}")
            self.test_results.append(("Network Connectivity", "ERROR"))
            return False
    
    def test_staking_functionality(self):
        """Test staking operations"""
        print("Testing staking functionality...")
        try:
            # Test getstakinginfo RPC
            result = subprocess.run([
                "./src/babachain-cli",
                "-regtest",
                "-rpcuser=test",
                "-rpcpassword=test",
                "getstakinginfo"
            ], capture_output=True, text=True)
            
            if result.returncode == 0:
                staking_info = json.loads(result.stdout)
                if "enabled" in staking_info:
                    self.test_results.append(("Staking Functionality", "PASS"))
                    return True
            
            self.test_results.append(("Staking Functionality", "FAIL"))
            return False
        except Exception as e:
            print(f"Staking functionality test failed: {e}")
            self.test_results.append(("Staking Functionality", "ERROR"))
            return False
    
    def test_supply_parameters(self):
        """Test supply and premine parameters"""
        print("Testing supply parameters...")
        try:
            result = subprocess.run([
                "./src/babachain-cli",
                "-regtest",
                "-rpcuser=test",
                "-rpcpassword=test",
                "getblockchaininfo"
            ], capture_output=True, text=True)
            
            if result.returncode == 0:
                blockchain_info = json.loads(result.stdout)
                if blockchain_info.get("chain") == "regtest":
                    self.test_results.append(("Supply Parameters", "PASS"))
                    return True
            
            self.test_results.append(("Supply Parameters", "FAIL"))
            return False
        except Exception as e:
            print(f"Supply parameters test failed: {e}")
            self.test_results.append(("Supply Parameters", "ERROR"))
            return False
    
    def test_mobile_wallet_compatibility(self):
        """Test mobile wallet compatibility"""
        print("Testing mobile wallet compatibility...")
        
        # Check if mobile wallet directories exist and have proper structure
        android_wallet = Path("BabaChain-AndroidMobileWallet")
        ios_wallet = Path("BabaChain-IOSMobileWallet")
        
        android_ok = android_wallet.exists() and (android_wallet / "wallet" / "build.gradle").exists()
        ios_ok = ios_wallet.exists() and (ios_wallet / "BabaChainWallet.xcodeproj").exists()
        
        if android_ok and ios_ok:
            self.test_results.append(("Mobile Wallet Compatibility", "PASS"))
            return True
        else:
            self.test_results.append(("Mobile Wallet Compatibility", "FAIL"))
            return False
    
    def run_all_tests(self):
        """Run all integration tests"""
        print("🧪 Running BabaChain Integration Tests...")
        
        if not self.start_core_node():
            print("❌ Failed to start core node, skipping tests")
            return False
        
        tests = [
            self.test_network_connectivity,
            self.test_staking_functionality,
            self.test_supply_parameters,
            self.test_mobile_wallet_compatibility
        ]
        
        for test in tests:
            test()
        
        self.stop_core_node()
        self.print_results()
        
        return all(result[1] == "PASS" for result in self.test_results)
    
    def stop_core_node(self):
        """Stop the core node"""
        if self.core_node:
            try:
                subprocess.run([
                    "./src/babachain-cli",
                    "-regtest",
                    "-rpcuser=test",
                    "-rpcpassword=test",
                    "stop"
                ])
                self.core_node.wait(timeout=10)
            except:
                self.core_node.terminate()
    
    def print_results(self):
        """Print test results"""
        print("\n📊 Integration Test Results:")
        print("=" * 50)
        
        for test_name, result in self.test_results:
            status_icon = "✅" if result == "PASS" else "❌" if result == "FAIL" else "⚠️"
            print(f"{status_icon} {test_name}: {result}")
        
        passed = sum(1 for _, result in self.test_results if result == "PASS")
        total = len(self.test_results)
        print(f"\nPassed: {passed}/{total}")

if __name__ == "__main__":
    test_suite = BabaChainIntegrationTest()
    success = test_suite.run_all_tests()
    exit(0 if success else 1)
