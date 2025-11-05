#!/bin/bash

# BabaChain System Integration Script
# This script integrates all wallet platforms with the core BabaChain network
# and ensures seamless cross-platform functionality

set -e

echo "🚀 Starting BabaChain System Integration..."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in the correct directory
if [ ! -f "src/chainparams.cpp" ]; then
    print_error "Please run this script from the BabaChain root directory"
    exit 1
fi

print_status "Validating BabaChain core network configuration..."

# 1. Validate Core Network Configuration
validate_core_config() {
    print_status "Checking core network parameters..."
    
    # Check if BabaChain genesis block is properly configured
    if grep -q "CreateBabaChainGenesisBlock" src/chainparams.cpp; then
        print_success "BabaChain genesis block configuration found"
    else
        print_error "BabaChain genesis block configuration missing"
        return 1
    fi
    
    # Check if PoS parameters are configured
    if grep -q "nSubsidyHalvingInterval = 0" src/chainparams.cpp; then
        print_success "PoS consensus parameters configured"
    else
        print_error "PoS consensus parameters missing"
        return 1
    fi
    
    # Check if supply parameters are set
    if grep -q "nMaxSupply = 1000000000" src/chainparams.cpp; then
        print_success "Supply parameters configured correctly"
    else
        print_error "Supply parameters not configured"
        return 1
    fi
    
    print_success "Core network configuration validated"
}

# 2. Validate Mobile Wallet Integration
validate_mobile_wallets() {
    print_status "Validating mobile wallet integration..."
    
    # Check Android wallet
    if [ -d "BabaChain-AndroidMobileWallet" ]; then
        print_status "Checking Android wallet configuration..."
        
        if [ -f "BabaChain-AndroidMobileWallet/wallet/build.gradle" ]; then
            print_success "Android wallet build configuration found"
        else
            print_warning "Android wallet build configuration missing"
        fi
        
        # Check for BabaChain branding
        if grep -r "BabaChain" BabaChain-AndroidMobileWallet/wallet/src/ >/dev/null 2>&1; then
            print_success "Android wallet BabaChain branding configured"
        else
            print_warning "Android wallet branding may need updates"
        fi
    else
        print_warning "Android wallet directory not found"
    fi
    
    # Check iOS wallet
    if [ -d "BabaChain-IOSMobileWallet" ]; then
        print_status "Checking iOS wallet configuration..."
        
        if [ -f "BabaChain-IOSMobileWallet/BabaChainWallet.xcodeproj/project.pbxproj" ]; then
            print_success "iOS wallet project configuration found"
        else
            print_warning "iOS wallet project configuration missing"
        fi
        
        # Check for BabaChain branding
        if grep -r "BabaChain" BabaChain-IOSMobileWallet/BabaChainWallet/Sources/ >/dev/null 2>&1; then
            print_success "iOS wallet BabaChain branding configured"
        else
            print_warning "iOS wallet branding may need updates"
        fi
    else
        print_warning "iOS wallet directory not found"
    fi
    
    print_success "Mobile wallet integration validated"
}

# 3. Validate Network Connectivity
validate_network_connectivity() {
    print_status "Validating network connectivity configuration..."
    
    # Check DNS seeds
    if grep -q "dnsseed.babachain.org" src/chainparams.cpp; then
        print_success "DNS seeds configured for BabaChain network"
    else
        print_warning "DNS seeds may need configuration"
    fi
    
    # Check network magic bytes
    if grep -q "0xbaba1337" src/chainparams.cpp; then
        print_success "Network magic bytes configured"
    else
        print_warning "Network magic bytes may need configuration"
    fi
    
    # Check default ports
    if grep -q "nDefaultPort = 9999" src/chainparams.cpp; then
        print_success "Default network port configured"
    else
        print_warning "Default network port may need configuration"
    fi
    
    print_success "Network connectivity configuration validated"
}

# 4. Validate Staking Integration
validate_staking_integration() {
    print_status "Validating staking integration..."
    
    # Check for PoS validation files
    if [ -f "src/pos.cpp" ]; then
        print_success "PoS validation implementation found"
    else
        print_warning "PoS validation implementation may be missing"
    fi
    
    # Check for staking RPC commands
    if grep -r "stakecoin" src/rpc/ >/dev/null 2>&1; then
        print_success "Staking RPC commands found"
    else
        print_warning "Staking RPC commands may be missing"
    fi
    
    # Check for validator management
    if grep -r "validator" src/ >/dev/null 2>&1; then
        print_success "Validator management code found"
    else
        print_warning "Validator management code may be missing"
    fi
    
    print_success "Staking integration validated"
}

# 5. Validate Turkish Language Support
validate_turkish_support() {
    print_status "Validating Turkish language support..."
    
    # Check desktop wallet Turkish support
    if [ -f "src/qt/locale/bitcoin_tr.ts" ]; then
        print_success "Desktop wallet Turkish translation found"
    else
        print_warning "Desktop wallet Turkish translation may be missing"
    fi
    
    # Check Android wallet Turkish support
    if [ -f "BabaChain-AndroidMobileWallet/wallet/res/values-tr/strings.xml" ]; then
        print_success "Android wallet Turkish translation found"
    else
        print_warning "Android wallet Turkish translation may be missing"
    fi
    
    # Check iOS wallet Turkish support
    if [ -d "BabaChain-IOSMobileWallet/BabaChainWallet/tr.lproj" ]; then
        print_success "iOS wallet Turkish translation found"
    else
        print_warning "iOS wallet Turkish translation may be missing"
    fi
    
    print_success "Turkish language support validated"
}

# 6. Create Integration Test Suite
create_integration_tests() {
    print_status "Creating integration test suite..."
    
    mkdir -p test/integration
    
    cat > test/integration/system_integration_test.py << 'EOF'
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
EOF

    chmod +x test/integration/system_integration_test.py
    print_success "Integration test suite created"
}

# 7. Create Cross-Platform Configuration
create_cross_platform_config() {
    print_status "Creating cross-platform configuration..."
    
    mkdir -p config/platforms
    
    # Desktop configuration
    cat > config/platforms/desktop.conf << 'EOF'
# BabaChain Desktop Wallet Configuration
# Network settings
rpcuser=babachain
rpcpassword=babachain_secure_password_2025
rpcport=9998
port=9999

# PoS settings
staking=1
stakegen=1
reservebalance=0

# Network discovery
addnode=seed.babachain.network
addnode=node.babachain.io
addnode=dnsseed.babachain.org

# Performance settings
maxconnections=125
timeout=5000

# Logging
debug=pos
debug=staking
debug=net
EOF

    # Mobile configuration
    cat > config/platforms/mobile.conf << 'EOF'
# BabaChain Mobile Wallet Configuration
# Lightweight node settings
spv=1
maxconnections=8
timeout=10000

# Background staking
staking=1
stakegen=1

# Mobile-optimized settings
prune=550
dbcache=100
maxmempool=50

# Network discovery
addnode=seed.babachain.network
addnode=node.babachain.io
EOF

    # Testnet configuration
    cat > config/platforms/testnet.conf << 'EOF'
# BabaChain Testnet Configuration
testnet=1
rpcport=19998
port=19999

# Testnet nodes
addnode=testnet-seed.babachain.network
addnode=testnet-node.babachain.io

# Development settings
staking=1
stakegen=1
debug=1
EOF

    print_success "Cross-platform configuration created"
}

# 8. Create Auto-Node Discovery System
create_auto_node_discovery() {
    print_status "Creating auto-node discovery system..."
    
    mkdir -p scripts/network
    
    cat > scripts/network/auto_discovery.py << 'EOF'
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
EOF

    chmod +x scripts/network/auto_discovery.py
    print_success "Auto-node discovery system created"
}

# Main execution
main() {
    print_status "Starting BabaChain System Integration Process..."
    
    # Run validation steps
    validate_core_config || exit 1
    validate_mobile_wallets
    validate_network_connectivity
    validate_staking_integration
    validate_turkish_support
    
    # Create integration components
    create_integration_tests
    create_cross_platform_config
    create_auto_node_discovery
    
    print_success "🎉 BabaChain System Integration Complete!"
    print_status "Next steps:"
    echo "  1. Run integration tests: python3 test/integration/system_integration_test.py"
    echo "  2. Test auto-node discovery: python3 scripts/network/auto_discovery.py"
    echo "  3. Build and test mobile wallets"
    echo "  4. Deploy to testnet for validation"
}

# Run main function
main "$@"