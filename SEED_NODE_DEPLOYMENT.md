# BabaChain Seed Node Deployment Guide

## Overview
This guide covers the deployment of BabaChain seed nodes for mainnet launch.

## Prerequisites
- Docker and Docker Compose installed
- OR Kubernetes cluster access
- Domain names configured for seed nodes
- SSL certificates (optional but recommended)

## Seed Nodes
5 seed nodes have been configured:

1. **seed1** - seed1.babachain.org (US-East)
2. **seed2** - seed2.babachain.org (EU-West)
3. **seed3** - seed3.babachain.org (Asia-Pacific)
4. **node1** - node1.babachain.network (US-West)
5. **node2** - node2.babachain.network (EU-Central)


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
Generated: 2025-11-05T18:04:24.461314
