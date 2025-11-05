#!/usr/bin/env python3
"""
BabaChain Mainnet Deployment Preparation Script
Configures mainnet parameters and creates the genesis block with 20M premine
"""

import json
import hashlib
import time
import subprocess
import os
from datetime import datetime
import logging

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class BabaChainMainnetPreparation:
    def __init__(self):
        self.mainnet_config = {}
        self.genesis_config = {}
        self.seed_nodes = []
        
    def configure_mainnet_parameters(self):
        """Configure mainnet parameters for BabaChain"""
        logger.info("🔧 Configuring BabaChain mainnet parameters...")
        
        # Mainnet launch timestamp (January 1, 2025, 00:00:00 UTC)
        mainnet_launch_time = int(datetime(2025, 1, 1, 0, 0, 0).timestamp())
        
        self.mainnet_config = {
            "network_name": "BabaChain Mainnet",
            "network_id": "main",
            "chain_id": 1,
            "genesis_timestamp": mainnet_launch_time,
            "network_magic": "0xbaba1337",
            "default_port": 9999,
            "rpc_port": 9998,
            "platform_p2p_port": 25656,
            "platform_http_port": 543,
            "consensus": {
                "algorithm": "proof_of_stake",
                "block_time": 150,  # 2.5 minutes
                "stake_min_age": 3600,  # 1 hour
                "stake_max_age": 86400,  # 24 hours
                "pos_target_spacing": 150
            },
            "supply": {
                "max_supply": 1000000000,  # 1B BabaChain
                "initial_supply": 210000000,  # 210M BabaChain
                "premine_amount": 20000000,  # 20M BabaChain (~9.5%)
                "staking_supply": 190000000,  # 190M for staking rewards
                "extended_staking": 790000000,  # 790M extended staking (210M to 1B)
                "reduction_interval": 20000000,  # Reward reduction every 20M coins
                "initial_block_reward": 200  # 200 BabaChain per block
            },
            "reward_schedule": [
                {"supply_range": [0, 20000000], "reward": 200},
                {"supply_range": [20000000, 40000000], "reward": 150},
                {"supply_range": [40000000, 60000000], "reward": 100},
                {"supply_range": [60000000, 80000000], "reward": 75},
                {"supply_range": [80000000, 100000000], "reward": 50},
                {"supply_range": [100000000, 120000000], "reward": 25},
                {"supply_range": [120000000, 210000000], "reward": 10}
            ]
        }
        
        logger.info("✅ Mainnet parameters configured")
        return self.mainnet_config
    
    def create_genesis_block_config(self):
        """Create genesis block configuration with 20M premine"""
        logger.info("🎯 Creating genesis block configuration...")
        
        # Genesis block configuration
        self.genesis_config = {
            "version": 1,
            "timestamp": self.mainnet_config["genesis_timestamp"],
            "difficulty": "0x1e0ffff0",  # Initial difficulty
            "nonce": 0,  # Will be calculated
            "premine": {
                "amount": 20000000,  # 20M BabaChain
                "address": "BabaChainFoundation2025Genesis",  # Will generate proper address
                "public_key": "04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f"
            },
            "coinbase_message": "BabaChain Genesis Block - New Era of Proof of Stake - January 2025",
            "merkle_root": "",  # Will be calculated
            "hash": ""  # Will be calculated
        }
        
        logger.info("✅ Genesis block configuration created")
        return self.genesis_config
    
    def setup_seed_nodes(self):
        """Set up initial network seed nodes"""
        logger.info("🌱 Setting up initial seed nodes...")
        
        self.seed_nodes = [
            {
                "hostname": "seed1.babachain.org",
                "ip": "TBD",  # Will be assigned during deployment
                "port": 9999,
                "location": "US-East",
                "type": "primary_seed"
            },
            {
                "hostname": "seed2.babachain.org", 
                "ip": "TBD",
                "port": 9999,
                "location": "EU-West",
                "type": "primary_seed"
            },
            {
                "hostname": "seed3.babachain.org",
                "ip": "TBD", 
                "port": 9999,
                "location": "Asia-Pacific",
                "type": "primary_seed"
            },
            {
                "hostname": "node1.babachain.network",
                "ip": "TBD",
                "port": 9999,
                "location": "US-West",
                "type": "bootstrap_node"
            },
            {
                "hostname": "node2.babachain.network",
                "ip": "TBD",
                "port": 9999,
                "location": "EU-Central", 
                "type": "bootstrap_node"
            }
        ]
        
        logger.info(f"✅ {len(self.seed_nodes)} seed nodes configured")
        return self.seed_nodes
    
    def generate_chainparams_mainnet(self):
        """Generate mainnet chainparams configuration"""
        logger.info("⚙️ Generating mainnet chainparams...")
        
        chainparams_mainnet = f"""
// BabaChain Mainnet Parameters
// Generated: {datetime.now().isoformat()}

class CMainParams : public CChainParams {{
public:
    CMainParams() {{
        strNetworkID = CBaseChainParams::MAIN;
        
        // BabaChain supply management - disable halving for PoS
        consensus.nSubsidyHalvingInterval = 0; // Disable halving for PoS - rewards managed differently
        
        // BabaChain supply parameters
        consensus.nMaxSupply = {self.mainnet_config['supply']['max_supply']} * COIN;       // 1B maximum possible supply (hard cap)
        consensus.nInitialSupply = {self.mainnet_config['supply']['initial_supply']} * COIN;    // 210M initial planned supply
        consensus.nPremineAmount = {self.mainnet_config['supply']['premine_amount']} * COIN;     // 20M premine (~9.5%)
        consensus.nStakingSupply = {self.mainnet_config['supply']['staking_supply']} * COIN;    // 190M initial staking rewards (~90.5%)
        consensus.nExtendedStaking = {self.mainnet_config['supply']['extended_staking']} * COIN;  // 790M extended staking (210M to 1B)
        consensus.nReductionInterval = {self.mainnet_config['supply']['reduction_interval']} * COIN; // Reward reduction every 20M coins
        consensus.nInitialBlockReward = {self.mainnet_config['supply']['initial_block_reward']} * COIN;     // Initial block reward: 200 BabaChain
        
        // Network timing parameters
        consensus.nPowTargetTimespan = 24 * 60 * 60; // BabaChain: 1 day (legacy)
        consensus.nPowTargetSpacing = {self.mainnet_config['consensus']['block_time']}; // BabaChain: 2.5 minutes (150 seconds)
        
        // PoW difficulty algorithms disabled for PoS
        consensus.nPowKGWHeight = 0; // Disabled - no KGW for PoS
        consensus.nPowDGWHeight = 0; // Disabled - no DGW for PoS
        
        // Network magic bytes: {self.mainnet_config['network_magic']}
        pchMessageStart[0] = 0xba;
        pchMessageStart[1] = 0xba;
        pchMessageStart[2] = 0x13;
        pchMessageStart[3] = 0x37;
        
        nDefaultPort = {self.mainnet_config['default_port']};
        nDefaultPlatformP2PPort = {self.mainnet_config['platform_p2p_port']};
        nDefaultPlatformHTTPPort = {self.mainnet_config['platform_http_port']};
        
        // Create BabaChain genesis block with 20M premine
        genesis = CreateBabaChainGenesisBlock({self.genesis_config['timestamp']}, 0, {self.genesis_config['difficulty']}, 1, {self.genesis_config['premine']['amount']} * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        
        // DNS seed nodes
"""
        
        for seed_node in self.seed_nodes:
            if seed_node['type'] == 'primary_seed':
                chainparams_mainnet += f'        vSeeds.emplace_back("{seed_node["hostname"]}.");\n'
        
        chainparams_mainnet += """
        
        // BabaChain addresses start with 'B' (base58 prefix 25)
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,25);
        // BabaChain script addresses start with 'C' (base58 prefix 28)  
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,28);
        // BabaChain private keys start with '7' or 'X'
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,204);
        // BabaChain BIP32 pubkeys start with 'xpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        // BabaChain BIP32 prvkeys start with 'xprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};
        
        // BabaChain BIP44 coin type is '5'
        nExtCoinType = 5;
    }
};
"""
        
        # Save to file
        with open("config/mainnet_chainparams.cpp", "w") as f:
            f.write(chainparams_mainnet)
        
        logger.info("✅ Mainnet chainparams generated")
        return chainparams_mainnet
    
    def create_mainnet_config_files(self):
        """Create mainnet configuration files"""
        logger.info("📄 Creating mainnet configuration files...")
        
        # Main configuration file
        mainnet_conf = f"""# BabaChain Mainnet Configuration
# Generated: {datetime.now().isoformat()}

# Network settings
rpcuser=babachain_mainnet
rpcpassword=CHANGE_THIS_PASSWORD_BEFORE_DEPLOYMENT
rpcport={self.mainnet_config['rpc_port']}
port={self.mainnet_config['default_port']}

# PoS settings
staking=1
stakegen=1
reservebalance=0

# Network discovery - Primary seed nodes
"""
        
        for seed_node in self.seed_nodes:
            mainnet_conf += f"addnode={seed_node['hostname']}:{seed_node['port']}\n"
        
        mainnet_conf += f"""
# Performance settings
maxconnections=125
timeout=5000
dbcache=300
maxmempool=300

# Security settings
rpcallowip=127.0.0.1
rpcbind=127.0.0.1

# Logging
debug=pos
debug=staking
debug=net

# Mainnet specific
testnet=0
regtest=0
"""
        
        with open("config/babachain-mainnet.conf", "w") as f:
            f.write(mainnet_conf)
        
        # Genesis block data file
        genesis_data = {
            "network": "mainnet",
            "genesis_block": self.genesis_config,
            "network_config": self.mainnet_config,
            "seed_nodes": self.seed_nodes,
            "deployment_checklist": [
                "Update DNS records for seed nodes",
                "Deploy seed nodes with proper IP addresses", 
                "Generate and distribute genesis block",
                "Update wallet configurations",
                "Announce mainnet launch",
                "Monitor network health"
            ]
        }
        
        with open("config/mainnet_genesis.json", "w") as f:
            json.dump(genesis_data, f, indent=2)
        
        logger.info("✅ Mainnet configuration files created")
    
    def create_deployment_scripts(self):
        """Create deployment scripts for mainnet"""
        logger.info("🚀 Creating deployment scripts...")
        
        # Node deployment script
        node_deploy_script = f"""#!/bin/bash
# BabaChain Mainnet Node Deployment Script

set -e

echo "🚀 Deploying BabaChain Mainnet Node..."

# Configuration
BABACHAIN_USER="babachain"
BABACHAIN_HOME="/home/$BABACHAIN_USER"
BABACHAIN_DATA="$BABACHAIN_HOME/.babachain"
BABACHAIN_CONF="$BABACHAIN_DATA/babachain.conf"

# Create user if not exists
if ! id "$BABACHAIN_USER" &>/dev/null; then
    echo "Creating BabaChain user..."
    useradd -m -s /bin/bash $BABACHAIN_USER
fi

# Create data directory
sudo -u $BABACHAIN_USER mkdir -p $BABACHAIN_DATA

# Copy configuration
sudo -u $BABACHAIN_USER cp config/babachain-mainnet.conf $BABACHAIN_CONF

# Set proper permissions
chmod 600 $BABACHAIN_CONF
chown $BABACHAIN_USER:$BABACHAIN_USER $BABACHAIN_CONF

# Install systemd service
cat > /etc/systemd/system/babachaind.service << 'EOF'
[Unit]
Description=BabaChain daemon
After=network.target

[Service]
Type=forking
User=babachain
Group=babachain
WorkingDirectory=/home/babachain
ExecStart=/usr/local/bin/babachaind -daemon -conf=/home/babachain/.babachain/babachain.conf
ExecStop=/usr/local/bin/babachain-cli stop
Restart=always
RestartSec=30
TimeoutStopSec=60
KillMode=process
PrivateTmp=true

[Install]
WantedBy=multi-user.target
EOF

# Enable and start service
systemctl daemon-reload
systemctl enable babachaind
systemctl start babachaind

echo "✅ BabaChain mainnet node deployed successfully!"
echo "📊 Check status with: systemctl status babachaind"
echo "📋 View logs with: journalctl -u babachaind -f"
"""
        
        with open("scripts/deploy-mainnet-node.sh", "w") as f:
            f.write(node_deploy_script)
        
        os.chmod("scripts/deploy-mainnet-node.sh", 0o755)
        
        # Genesis block generation script
        genesis_script = f"""#!/bin/bash
# BabaChain Genesis Block Generation Script

set -e

echo "🎯 Generating BabaChain Genesis Block..."

# Build babachaind if not exists
if [ ! -f "src/babachaind" ]; then
    echo "Building BabaChain daemon..."
    make -j$(nproc) babachaind
fi

# Generate genesis block
echo "Generating genesis block with 20M premine..."
./src/babachaind -printtoconsole -regtest -gen=0 -connect=0 -listen=0 -rpcuser=genesis -rpcpassword=genesis &
DAEMON_PID=$!

# Wait for daemon to start
sleep 5

# Generate genesis block
GENESIS_HASH=$(./src/babachain-cli -regtest -rpcuser=genesis -rpcpassword=genesis getblockhash 0)
GENESIS_BLOCK=$(./src/babachain-cli -regtest -rpcuser=genesis -rpcpassword=genesis getblock $GENESIS_HASH)

echo "Genesis Block Hash: $GENESIS_HASH"
echo "Genesis Block Data: $GENESIS_BLOCK"

# Save genesis data
echo "$GENESIS_BLOCK" > config/mainnet_genesis_block.json

# Stop daemon
kill $DAEMON_PID

echo "✅ Genesis block generated successfully!"
echo "📄 Genesis data saved to: config/mainnet_genesis_block.json"
"""
        
        with open("scripts/generate-genesis.sh", "w") as f:
            f.write(genesis_script)
        
        os.chmod("scripts/generate-genesis.sh", 0o755)
        
        logger.info("✅ Deployment scripts created")
    
    def create_monitoring_setup(self):
        """Create monitoring and health check setup"""
        logger.info("📊 Creating monitoring setup...")
        
        # Health check script
        health_check_script = f"""#!/usr/bin/env python3
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
        self.rpc_port = {self.mainnet_config['rpc_port']}
        
    def check_node_status(self):
        try:
            result = subprocess.run([
                "babachain-cli",
                f"-rpcuser={{self.rpc_user}}",
                f"-rpcpassword={{self.rpc_password}}",
                f"-rpcport={{self.rpc_port}}",
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
                f"-rpcuser={{self.rpc_user}}",
                f"-rpcpassword={{self.rpc_password}}",
                f"-rpcport={{self.rpc_port}}",
                "getstakinginfo"
            ], capture_output=True, text=True)
            
            if result.returncode == 0:
                return json.loads(result.stdout)
            return None
        except:
            return None
    
    def run_health_check(self):
        print(f"🏥 BabaChain Health Check - {{datetime.now().isoformat()}}")
        print("=" * 60)
        
        # Check node status
        network_info = self.check_node_status()
        if network_info:
            print(f"✅ Node Status: Online")
            print(f"   Version: {{network_info.get('version', 'Unknown')}}")
            print(f"   Connections: {{network_info.get('connections', 0)}}")
            print(f"   Network Active: {{network_info.get('networkactive', False)}}")
        else:
            print("❌ Node Status: Offline or unreachable")
        
        # Check staking status
        staking_info = self.check_staking_status()
        if staking_info:
            print(f"✅ Staking Status: {{staking_info.get('enabled', False)}}")
            print(f"   Staking: {{staking_info.get('staking', False)}}")
            print(f"   Expected Time: {{staking_info.get('expectedtime', 'Unknown')}}")
        else:
            print("❌ Staking Status: Unable to retrieve")
        
        print("=" * 60)

if __name__ == "__main__":
    monitor = BabaChainHealthMonitor()
    monitor.run_health_check()
"""
        
        with open("scripts/health-check.py", "w") as f:
            f.write(health_check_script)
        
        os.chmod("scripts/health-check.py", 0o755)
        
        # Monitoring cron job
        cron_job = """# BabaChain Mainnet Monitoring Cron Jobs
# Check health every 5 minutes
*/5 * * * * /usr/local/bin/python3 /opt/babachain/scripts/health-check.py >> /var/log/babachain-health.log 2>&1

# Daily backup
0 2 * * * /opt/babachain/scripts/backup-wallet.sh >> /var/log/babachain-backup.log 2>&1
"""
        
        with open("config/babachain-cron", "w") as f:
            f.write(cron_job)
        
        logger.info("✅ Monitoring setup created")
    
    def prepare_mainnet_deployment(self):
        """Complete mainnet deployment preparation"""
        logger.info("🚀 Starting BabaChain Mainnet Deployment Preparation...")
        
        # Create necessary directories
        os.makedirs("config", exist_ok=True)
        os.makedirs("scripts", exist_ok=True)
        
        # Run all preparation steps
        self.configure_mainnet_parameters()
        self.create_genesis_block_config()
        self.setup_seed_nodes()
        self.generate_chainparams_mainnet()
        self.create_mainnet_config_files()
        self.create_deployment_scripts()
        self.create_monitoring_setup()
        
        # Create deployment summary
        deployment_summary = {
            "preparation_date": datetime.now().isoformat(),
            "mainnet_launch_date": datetime.fromtimestamp(self.mainnet_config["genesis_timestamp"]).isoformat(),
            "network_config": self.mainnet_config,
            "genesis_config": self.genesis_config,
            "seed_nodes": len(self.seed_nodes),
            "files_created": [
                "config/mainnet_chainparams.cpp",
                "config/babachain-mainnet.conf", 
                "config/mainnet_genesis.json",
                "scripts/deploy-mainnet-node.sh",
                "scripts/generate-genesis.sh",
                "scripts/health-check.py",
                "config/babachain-cron"
            ],
            "deployment_checklist": [
                "✅ Configure mainnet parameters",
                "✅ Create genesis block configuration", 
                "✅ Set up seed nodes",
                "✅ Generate chainparams",
                "✅ Create configuration files",
                "✅ Create deployment scripts",
                "✅ Set up monitoring",
                "⏳ Deploy seed nodes",
                "⏳ Generate actual genesis block",
                "⏳ Update DNS records",
                "⏳ Launch mainnet",
                "⏳ Monitor network health"
            ]
        }
        
        with open("mainnet_deployment_summary.json", "w") as f:
            json.dump(deployment_summary, f, indent=2)
        
        print("\n" + "="*60)
        print("🎉 BABACHAIN MAINNET DEPLOYMENT PREPARATION COMPLETE!")
        print("="*60)
        print(f"📅 Mainnet Launch Date: {datetime.fromtimestamp(self.mainnet_config['genesis_timestamp']).strftime('%Y-%m-%d %H:%M:%S UTC')}")
        print(f"💰 Premine Amount: {self.mainnet_config['supply']['premine_amount']:,} BabaChain")
        print(f"🌱 Seed Nodes: {len(self.seed_nodes)} configured")
        print(f"📄 Files Created: {len(deployment_summary['files_created'])}")
        
        print("\n📋 Next Steps:")
        print("  1. Review configuration files in config/")
        print("  2. Deploy seed nodes using scripts/deploy-mainnet-node.sh")
        print("  3. Generate genesis block with scripts/generate-genesis.sh")
        print("  4. Update DNS records for seed nodes")
        print("  5. Launch mainnet on scheduled date")
        print("  6. Monitor network health with scripts/health-check.py")
        
        print(f"\n📊 Deployment summary saved to: mainnet_deployment_summary.json")
        
        logger.info("✅ Mainnet deployment preparation completed successfully!")

def main():
    """Main function"""
    preparation = BabaChainMainnetPreparation()
    preparation.prepare_mainnet_deployment()

if __name__ == "__main__":
    main()