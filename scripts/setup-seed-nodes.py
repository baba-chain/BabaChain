#!/usr/bin/env python3
"""
BabaChain Seed Node Setup Script
Sets up initial network seed nodes for mainnet deployment
"""

import json
import subprocess
import time
import os
from datetime import datetime
import logging

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class BabaChainSeedNodeSetup:
    def __init__(self):
        self.seed_nodes = []
        self.deployment_config = {}
        
    def load_deployment_config(self):
        """Load deployment configuration"""
        try:
            with open("mainnet_deployment_summary.json", "r") as f:
                self.deployment_config = json.load(f)
                logger.info("✅ Deployment configuration loaded")
                return True
        except FileNotFoundError:
            logger.error("❌ Deployment configuration not found. Run prepare-mainnet.py first.")
            return False
    
    def create_seed_node_configs(self):
        """Create individual configuration files for each seed node"""
        logger.info("🌱 Creating seed node configurations...")
        
        os.makedirs("config/seed-nodes", exist_ok=True)
        
        seed_nodes = [
            {
                "name": "seed1",
                "hostname": "seed1.babachain.org",
                "location": "US-East",
                "rpc_port": 9998,
                "p2p_port": 9999
            },
            {
                "name": "seed2", 
                "hostname": "seed2.babachain.org",
                "location": "EU-West",
                "rpc_port": 9998,
                "p2p_port": 9999
            },
            {
                "name": "seed3",
                "hostname": "seed3.babachain.org", 
                "location": "Asia-Pacific",
                "rpc_port": 9998,
                "p2p_port": 9999
            },
            {
                "name": "node1",
                "hostname": "node1.babachain.network",
                "location": "US-West", 
                "rpc_port": 9998,
                "p2p_port": 9999
            },
            {
                "name": "node2",
                "hostname": "node2.babachain.network",
                "location": "EU-Central",
                "rpc_port": 9998,
                "p2p_port": 9999
            }
        ]
        
        for node in seed_nodes:
            config_content = f"""# BabaChain Seed Node Configuration - {node['name']}
# Location: {node['location']}
# Hostname: {node['hostname']}
# Generated: {datetime.now().isoformat()}

# Network settings
rpcuser=babachain_seed_{node['name']}
rpcpassword=CHANGE_THIS_SECURE_PASSWORD_{node['name'].upper()}
rpcport={node['rpc_port']}
port={node['p2p_port']}
rpcbind=0.0.0.0
rpcallowip=0.0.0.0/0

# Seed node specific settings
listen=1
discover=1
upnp=0
maxconnections=200
timeout=5000

# PoS settings (seed nodes should stake)
staking=1
stakegen=1
reservebalance=1000

# Performance settings for seed nodes
dbcache=512
maxmempool=500
mempoolexpiry=72

# Connect to other seed nodes
"""
            
            # Add connections to other seed nodes
            for other_node in seed_nodes:
                if other_node['name'] != node['name']:
                    config_content += f"addnode={other_node['hostname']}:{other_node['p2p_port']}\n"
            
            config_content += f"""
# Logging for seed nodes
debug=net
debug=addrman
debug=pos
debug=staking

# Security settings
rpcssl=0
rpcsslcertificatechainfile=server.cert
rpcsslprivatekeyfile=server.pem

# Mainnet settings
testnet=0
regtest=0

# Seed node identification
uacomment=BabaChain-Seed-{node['name']}-{node['location']}
"""
            
            config_file = f"config/seed-nodes/{node['name']}.conf"
            with open(config_file, "w") as f:
                f.write(config_content)
            
            logger.info(f"✅ Created config for {node['name']} ({node['location']})")
        
        self.seed_nodes = seed_nodes
        logger.info(f"✅ {len(seed_nodes)} seed node configurations created")
    
    def create_docker_compose(self):
        """Create Docker Compose configuration for seed nodes"""
        logger.info("🐳 Creating Docker Compose configuration...")
        
        docker_compose = {
            "version": "3.8",
            "services": {},
            "networks": {
                "babachain-network": {
                    "driver": "bridge"
                }
            },
            "volumes": {}
        }
        
        for node in self.seed_nodes:
            service_name = f"babachain-{node['name']}"
            
            docker_compose["services"][service_name] = {
                "image": "babachain/babachain:latest",
                "container_name": service_name,
                "restart": "unless-stopped",
                "ports": [
                    f"{node['p2p_port']}:{node['p2p_port']}",
                    f"{node['rpc_port']}:{node['rpc_port']}"
                ],
                "volumes": [
                    f"{node['name']}-data:/home/babachain/.babachain",
                    f"./config/seed-nodes/{node['name']}.conf:/home/babachain/.babachain/babachain.conf:ro"
                ],
                "networks": ["babachain-network"],
                "environment": {
                    "BABACHAIN_NETWORK": "mainnet",
                    "BABACHAIN_NODE_TYPE": "seed"
                },
                "healthcheck": {
                    "test": ["CMD", "babachain-cli", "getnetworkinfo"],
                    "interval": "30s",
                    "timeout": "10s",
                    "retries": 3,
                    "start_period": "60s"
                }
            }
            
            docker_compose["volumes"][f"{node['name']}-data"] = {}
        
        with open("docker-compose.seed-nodes.yml", "w") as f:
            import yaml
            yaml.dump(docker_compose, f, default_flow_style=False, indent=2)
        
        logger.info("✅ Docker Compose configuration created")
    
    def create_kubernetes_manifests(self):
        """Create Kubernetes manifests for seed nodes"""
        logger.info("☸️ Creating Kubernetes manifests...")
        
        os.makedirs("k8s", exist_ok=True)
        
        # Namespace
        namespace_manifest = {
            "apiVersion": "v1",
            "kind": "Namespace",
            "metadata": {
                "name": "babachain",
                "labels": {
                    "name": "babachain"
                }
            }
        }
        
        with open("k8s/namespace.yaml", "w") as f:
            import yaml
            yaml.dump(namespace_manifest, f, default_flow_style=False)
        
        # ConfigMap for each seed node
        for node in self.seed_nodes:
            with open(f"config/seed-nodes/{node['name']}.conf", "r") as f:
                config_content = f.read()
            
            configmap_manifest = {
                "apiVersion": "v1",
                "kind": "ConfigMap",
                "metadata": {
                    "name": f"babachain-{node['name']}-config",
                    "namespace": "babachain"
                },
                "data": {
                    "babachain.conf": config_content
                }
            }
            
            with open(f"k8s/configmap-{node['name']}.yaml", "w") as f:
                import yaml
                yaml.dump(configmap_manifest, f, default_flow_style=False)
            
            # Deployment for each seed node
            deployment_manifest = {
                "apiVersion": "apps/v1",
                "kind": "Deployment",
                "metadata": {
                    "name": f"babachain-{node['name']}",
                    "namespace": "babachain",
                    "labels": {
                        "app": "babachain",
                        "node-type": "seed",
                        "node-name": node['name']
                    }
                },
                "spec": {
                    "replicas": 1,
                    "selector": {
                        "matchLabels": {
                            "app": "babachain",
                            "node-name": node['name']
                        }
                    },
                    "template": {
                        "metadata": {
                            "labels": {
                                "app": "babachain",
                                "node-type": "seed", 
                                "node-name": node['name']
                            }
                        },
                        "spec": {
                            "containers": [{
                                "name": "babachain",
                                "image": "babachain/babachain:latest",
                                "ports": [
                                    {
                                        "containerPort": node['p2p_port'],
                                        "name": "p2p"
                                    },
                                    {
                                        "containerPort": node['rpc_port'],
                                        "name": "rpc"
                                    }
                                ],
                                "volumeMounts": [
                                    {
                                        "name": "config",
                                        "mountPath": "/home/babachain/.babachain/babachain.conf",
                                        "subPath": "babachain.conf"
                                    },
                                    {
                                        "name": "data",
                                        "mountPath": "/home/babachain/.babachain"
                                    }
                                ],
                                "resources": {
                                    "requests": {
                                        "memory": "1Gi",
                                        "cpu": "500m"
                                    },
                                    "limits": {
                                        "memory": "2Gi", 
                                        "cpu": "1000m"
                                    }
                                },
                                "livenessProbe": {
                                    "exec": {
                                        "command": ["babachain-cli", "getnetworkinfo"]
                                    },
                                    "initialDelaySeconds": 60,
                                    "periodSeconds": 30
                                }
                            }],
                            "volumes": [
                                {
                                    "name": "config",
                                    "configMap": {
                                        "name": f"babachain-{node['name']}-config"
                                    }
                                },
                                {
                                    "name": "data",
                                    "persistentVolumeClaim": {
                                        "claimName": f"babachain-{node['name']}-data"
                                    }
                                }
                            ]
                        }
                    }
                }
            }
            
            with open(f"k8s/deployment-{node['name']}.yaml", "w") as f:
                import yaml
                yaml.dump(deployment_manifest, f, default_flow_style=False)
            
            # Service for each seed node
            service_manifest = {
                "apiVersion": "v1",
                "kind": "Service",
                "metadata": {
                    "name": f"babachain-{node['name']}",
                    "namespace": "babachain"
                },
                "spec": {
                    "selector": {
                        "app": "babachain",
                        "node-name": node['name']
                    },
                    "ports": [
                        {
                            "name": "p2p",
                            "port": node['p2p_port'],
                            "targetPort": node['p2p_port'],
                            "protocol": "TCP"
                        },
                        {
                            "name": "rpc",
                            "port": node['rpc_port'],
                            "targetPort": node['rpc_port'],
                            "protocol": "TCP"
                        }
                    ],
                    "type": "LoadBalancer"
                }
            }
            
            with open(f"k8s/service-{node['name']}.yaml", "w") as f:
                import yaml
                yaml.dump(service_manifest, f, default_flow_style=False)
            
            # PersistentVolumeClaim for each seed node
            pvc_manifest = {
                "apiVersion": "v1",
                "kind": "PersistentVolumeClaim",
                "metadata": {
                    "name": f"babachain-{node['name']}-data",
                    "namespace": "babachain"
                },
                "spec": {
                    "accessModes": ["ReadWriteOnce"],
                    "resources": {
                        "requests": {
                            "storage": "100Gi"
                        }
                    }
                }
            }
            
            with open(f"k8s/pvc-{node['name']}.yaml", "w") as f:
                import yaml
                yaml.dump(pvc_manifest, f, default_flow_style=False)
        
        logger.info("✅ Kubernetes manifests created")
    
    def create_monitoring_setup(self):
        """Create monitoring setup for seed nodes"""
        logger.info("📊 Creating monitoring setup...")
        
        # Prometheus configuration
        prometheus_config = {
            "global": {
                "scrape_interval": "15s"
            },
            "scrape_configs": [
                {
                    "job_name": "babachain-seed-nodes",
                    "static_configs": [
                        {
                            "targets": [f"{node['hostname']}:{node['rpc_port']}" for node in self.seed_nodes]
                        }
                    ],
                    "metrics_path": "/metrics",
                    "scrape_interval": "30s"
                }
            ]
        }
        
        with open("config/prometheus.yml", "w") as f:
            import yaml
            yaml.dump(prometheus_config, f, default_flow_style=False)
        
        # Grafana dashboard
        grafana_dashboard = {
            "dashboard": {
                "title": "BabaChain Seed Nodes",
                "panels": [
                    {
                        "title": "Node Status",
                        "type": "stat",
                        "targets": [
                            {
                                "expr": "up{job='babachain-seed-nodes'}",
                                "legendFormat": "{{instance}}"
                            }
                        ]
                    },
                    {
                        "title": "Network Connections",
                        "type": "graph",
                        "targets": [
                            {
                                "expr": "babachain_connections{job='babachain-seed-nodes'}",
                                "legendFormat": "{{instance}}"
                            }
                        ]
                    },
                    {
                        "title": "Block Height",
                        "type": "graph", 
                        "targets": [
                            {
                                "expr": "babachain_block_height{job='babachain-seed-nodes'}",
                                "legendFormat": "{{instance}}"
                            }
                        ]
                    }
                ]
            }
        }
        
        with open("config/grafana-dashboard.json", "w") as f:
            json.dump(grafana_dashboard, f, indent=2)
        
        logger.info("✅ Monitoring setup created")
    
    def create_deployment_guide(self):
        """Create deployment guide"""
        logger.info("📖 Creating deployment guide...")
        
        deployment_guide = f"""# BabaChain Seed Node Deployment Guide

## Overview
This guide covers the deployment of BabaChain seed nodes for mainnet launch.

## Prerequisites
- Docker and Docker Compose installed
- OR Kubernetes cluster access
- Domain names configured for seed nodes
- SSL certificates (optional but recommended)

## Seed Nodes
{len(self.seed_nodes)} seed nodes have been configured:

"""
        
        for i, node in enumerate(self.seed_nodes, 1):
            deployment_guide += f"{i}. **{node['name']}** - {node['hostname']} ({node['location']})\n"
        
        deployment_guide += f"""

## Docker Deployment

### 1. Build BabaChain Docker Image
```bash
# Build the BabaChain Docker image
docker build -t babachain/babachain:latest .
```

### 2. Deploy Seed Nodes
```bash
# Deploy all seed nodes using Docker Compose
docker-compose -f docker-compose.seed-nodes.yml up -d

# Check status
docker-compose -f docker-compose.seed-nodes.yml ps

# View logs
docker-compose -f docker-compose.seed-nodes.yml logs -f
```

### 3. Individual Node Management
```bash
# Start specific node
docker-compose -f docker-compose.seed-nodes.yml up -d babachain-seed1

# Stop specific node
docker-compose -f docker-compose.seed-nodes.yml stop babachain-seed1

# View node logs
docker-compose -f docker-compose.seed-nodes.yml logs -f babachain-seed1
```

## Kubernetes Deployment

### 1. Create Namespace
```bash
kubectl apply -f k8s/namespace.yaml
```

### 2. Deploy ConfigMaps
```bash
kubectl apply -f k8s/configmap-*.yaml
```

### 3. Deploy PersistentVolumeClaims
```bash
kubectl apply -f k8s/pvc-*.yaml
```

### 4. Deploy Seed Nodes
```bash
kubectl apply -f k8s/deployment-*.yaml
kubectl apply -f k8s/service-*.yaml
```

### 5. Check Status
```bash
# Check pods
kubectl get pods -n babachain

# Check services
kubectl get services -n babachain

# View logs
kubectl logs -n babachain -l app=babachain -f
```

## Configuration

### Security
1. **Change default passwords** in all configuration files
2. **Set up SSL certificates** for RPC connections
3. **Configure firewall rules** to allow only necessary ports
4. **Enable fail2ban** for additional security

### DNS Configuration
Update DNS records for seed nodes:
```
seed1.babachain.org    A    <IP_ADDRESS>
seed2.babachain.org    A    <IP_ADDRESS>
seed3.babachain.org    A    <IP_ADDRESS>
node1.babachain.network A   <IP_ADDRESS>
node2.babachain.network A   <IP_ADDRESS>
```

### Monitoring
1. Deploy Prometheus: `docker run -p 9090:9090 -v $(pwd)/config/prometheus.yml:/etc/prometheus/prometheus.yml prom/prometheus`
2. Deploy Grafana: `docker run -p 3000:3000 grafana/grafana`
3. Import dashboard from `config/grafana-dashboard.json`

## Health Checks

### Manual Health Check
```bash
# Check node status
babachain-cli getnetworkinfo

# Check staking status
babachain-cli getstakinginfo

# Check connections
babachain-cli getpeerinfo
```

### Automated Monitoring
```bash
# Run health check script
python3 scripts/health-check.py

# Set up cron job for regular checks
crontab config/babachain-cron
```

## Troubleshooting

### Common Issues
1. **Node not connecting**: Check firewall and network configuration
2. **Sync issues**: Verify genesis block and network parameters
3. **High memory usage**: Adjust dbcache and maxmempool settings
4. **Staking not working**: Check wallet unlock and balance

### Log Analysis
```bash
# Docker logs
docker-compose logs babachain-seed1

# Kubernetes logs
kubectl logs -n babachain deployment/babachain-seed1

# System logs
journalctl -u babachaind -f
```

## Maintenance

### Regular Tasks
1. **Monitor disk space** and clean old logs
2. **Update BabaChain software** when new versions are released
3. **Backup wallet files** regularly
4. **Monitor network health** and performance

### Backup Procedures
```bash
# Backup wallet
cp ~/.babachain/wallet.dat ~/backup/wallet-$(date +%Y%m%d).dat

# Backup configuration
cp ~/.babachain/babachain.conf ~/backup/babachain-$(date +%Y%m%d).conf
```

## Support
For support and questions:
- GitHub: https://github.com/Baba-Chain/BabaChain
- Discord: https://discord.gg/babachain
- Email: support@babachain.org

---
Generated: {datetime.now().isoformat()}
"""
        
        with open("SEED_NODE_DEPLOYMENT.md", "w") as f:
            f.write(deployment_guide)
        
        logger.info("✅ Deployment guide created")
    
    def setup_seed_nodes(self):
        """Complete seed node setup process"""
        logger.info("🚀 Starting BabaChain Seed Node Setup...")
        
        if not self.load_deployment_config():
            return False
        
        self.create_seed_node_configs()
        
        try:
            self.create_docker_compose()
        except ImportError:
            logger.warning("⚠️ PyYAML not installed, skipping Docker Compose generation")
        
        try:
            self.create_kubernetes_manifests()
        except ImportError:
            logger.warning("⚠️ PyYAML not installed, skipping Kubernetes manifests")
        
        self.create_monitoring_setup()
        self.create_deployment_guide()
        
        print("\n" + "="*60)
        print("🎉 BABACHAIN SEED NODE SETUP COMPLETE!")
        print("="*60)
        print(f"🌱 Seed Nodes Configured: {len(self.seed_nodes)}")
        print("📄 Files Created:")
        print("   - config/seed-nodes/*.conf (node configurations)")
        print("   - docker-compose.seed-nodes.yml (Docker deployment)")
        print("   - k8s/*.yaml (Kubernetes manifests)")
        print("   - config/prometheus.yml (monitoring)")
        print("   - SEED_NODE_DEPLOYMENT.md (deployment guide)")
        
        print("\n📋 Next Steps:")
        print("  1. Review and update passwords in config files")
        print("  2. Set up DNS records for seed node hostnames")
        print("  3. Deploy seed nodes using Docker or Kubernetes")
        print("  4. Configure monitoring and alerting")
        print("  5. Test network connectivity between nodes")
        
        logger.info("✅ Seed node setup completed successfully!")
        return True

def main():
    """Main function"""
    setup = BabaChainSeedNodeSetup()
    success = setup.setup_seed_nodes()
    
    if not success:
        exit(1)

if __name__ == "__main__":
    main()