# 🐳 BabaChain Docker Image

[![Docker Pulls](https://img.shields.io/docker/pulls/babachain/babachain)](https://hub.docker.com/r/babachain/babachain)
[![Docker Image Size](https://img.shields.io/docker/image-size/babachain/babachain/latest)](https://hub.docker.com/r/babachain/babachain)
[![Docker Image Version](https://img.shields.io/docker/v/babachain/babachain?sort=semver)](https://hub.docker.com/r/babachain/babachain)

Official Docker image for BabaChain - A Proof-of-Stake cryptocurrency with advanced staking features.

## 🚀 Quick Start

### Run BabaChain Node
```bash
docker run -d --name babachain \
  -p 9999:9999 \
  -p 9998:9998 \
  -v babachain-data:/home/babachain/.babachain \
  babachain/babachain:latest
```

### Run with Custom Configuration
```bash
docker run -d --name babachain \
  -p 9999:9999 \
  -p 9998:9998 \
  -v babachain-data:/home/babachain/.babachain \
  -v /path/to/babachain.conf:/home/babachain/.babachain/babachain.conf \
  babachain/babachain:latest
```

### Docker Compose
```yaml
version: '3.8'
services:
  babachain:
    image: babachain/babachain:latest
    container_name: babachain
    ports:
      - "9999:9999"
      - "9998:9998"
    volumes:
      - babachain-data:/home/babachain/.babachain
    restart: unless-stopped
    environment:
      - BABACHAIN_RPCUSER=your_rpc_user
      - BABACHAIN_RPCPASSWORD=your_secure_password

volumes:
  babachain-data:
```

## 🏗️ Supported Architectures

This image supports multiple architectures:

- `linux/amd64` - x86_64 Linux systems
- `linux/arm64` - ARM64 systems (Raspberry Pi 4, AWS Graviton, Apple M1/M2)

Docker will automatically pull the correct architecture for your system.

## 🔧 Configuration

### Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `BABACHAIN_RPCUSER` | RPC username | `babachain_user` |
| `BABACHAIN_RPCPASSWORD` | RPC password | `random_generated` |
| `BABACHAIN_RPCPORT` | RPC port | `9998` |
| `BABACHAIN_PORT` | P2P port | `9999` |
| `BABACHAIN_STAKING` | Enable staking | `1` |
| `BABACHAIN_TESTNET` | Use testnet | `0` |

### Volume Mounts

- `/home/babachain/.babachain` - BabaChain data directory (blockchain, wallet, config)

### Ports

- `9999/tcp` - P2P network port
- `9998/tcp` - RPC port (bind to localhost only by default)

## 📋 Usage Examples

### Basic Node
```bash
docker run -d --name babachain-node \
  -p 9999:9999 \
  -v babachain-data:/home/babachain/.babachain \
  babachain/babachain:latest
```

### Staking Node
```bash
docker run -d --name babachain-staking \
  -p 9999:9999 \
  -p 127.0.0.1:9998:9998 \
  -v babachain-data:/home/babachain/.babachain \
  -e BABACHAIN_STAKING=1 \
  -e BABACHAIN_RPCUSER=staker \
  -e BABACHAIN_RPCPASSWORD=secure_password_here \
  babachain/babachain:latest
```

### Testnet Node
```bash
docker run -d --name babachain-testnet \
  -p 19999:19999 \
  -p 127.0.0.1:19998:19998 \
  -v babachain-testnet-data:/home/babachain/.babachain \
  -e BABACHAIN_TESTNET=1 \
  -e BABACHAIN_PORT=19999 \
  -e BABACHAIN_RPCPORT=19998 \
  babachain/babachain:latest
```

## 🔍 Monitoring

### Check Node Status
```bash
# Check if container is running
docker ps | grep babachain

# View logs
docker logs babachain

# Follow logs in real-time
docker logs -f babachain

# Execute commands inside container
docker exec babachain babachain-cli getblockchaininfo
docker exec babachain babachain-cli getstakinginfo
```

### Health Check
```bash
# Check if RPC is responding
docker exec babachain babachain-cli getinfo

# Check peer connections
docker exec babachain babachain-cli getpeerinfo

# Check wallet status
docker exec babachain babachain-cli getwalletinfo
```

## 🛡️ Security

### Best Practices

1. **Never expose RPC port to public internet**
   ```bash
   # Good: Bind to localhost only
   -p 127.0.0.1:9998:9998
   
   # Bad: Expose to all interfaces
   -p 9998:9998
   ```

2. **Use strong RPC credentials**
   ```bash
   -e BABACHAIN_RPCUSER=your_secure_username
   -e BABACHAIN_RPCPASSWORD=your_very_secure_password
   ```

3. **Regular backups**
   ```bash
   # Backup wallet
   docker exec babachain babachain-cli backupwallet /home/babachain/.babachain/wallet_backup.dat
   
   # Copy backup to host
   docker cp babachain:/home/babachain/.babachain/wallet_backup.dat ./wallet_backup.dat
   ```

4. **Use Docker secrets for sensitive data**
   ```yaml
   version: '3.8'
   services:
     babachain:
       image: babachain/babachain:latest
       secrets:
         - rpc_password
       environment:
         - BABACHAIN_RPCPASSWORD_FILE=/run/secrets/rpc_password
   
   secrets:
     rpc_password:
       file: ./rpc_password.txt
   ```

## 🔄 Updates

### Update to Latest Version
```bash
# Stop current container
docker stop babachain

# Pull latest image
docker pull babachain/babachain:latest

# Remove old container
docker rm babachain

# Start with new image
docker run -d --name babachain \
  -p 9999:9999 \
  -p 127.0.0.1:9998:9998 \
  -v babachain-data:/home/babachain/.babachain \
  babachain/babachain:latest
```

### Backup Before Update
```bash
# Create backup
docker exec babachain babachain-cli stop
docker run --rm -v babachain-data:/data -v $(pwd):/backup alpine tar czf /backup/babachain-backup-$(date +%Y%m%d).tar.gz /data

# Restore if needed
docker run --rm -v babachain-data:/data -v $(pwd):/backup alpine tar xzf /backup/babachain-backup-YYYYMMDD.tar.gz -C /
```

## 🐛 Troubleshooting

### Common Issues

#### Container Won't Start
```bash
# Check logs for errors
docker logs babachain

# Check if ports are already in use
netstat -tulpn | grep :9999
netstat -tulpn | grep :9998

# Check disk space
df -h
```

#### Blockchain Sync Issues
```bash
# Check peer connections
docker exec babachain babachain-cli getpeerinfo

# Add manual peers
docker exec babachain babachain-cli addnode "seed1.babachain.org:9999" "add"

# Check sync progress
docker exec babachain babachain-cli getblockchaininfo
```

#### RPC Connection Issues
```bash
# Test RPC connection
docker exec babachain babachain-cli getinfo

# Check RPC configuration
docker exec babachain cat /home/babachain/.babachain/babachain.conf
```

## 📚 Documentation

- [BabaChain GitHub](https://github.com/BabaChain/BabaChain)
- [Ubuntu Deployment Guide](https://github.com/BabaChain/BabaChain/blob/main/BABACHAIN_UBUNTU_DEPLOYMENT.md)
- [macOS Deployment Guide](https://github.com/BabaChain/BabaChain/blob/main/BABACHAIN_MACOS_DEPLOYMENT.md)
- [Windows Deployment Guide](https://github.com/BabaChain/BabaChain/blob/main/BABACHAIN_WINDOWS_DEPLOYMENT.md)

## 🆘 Support

- **GitHub Issues**: https://github.com/BabaChain/BabaChain/issues
- **Discord**: https://discord.gg/babachain
- **Telegram**: https://t.me/babachainofficial

## 📄 License

This Docker image is licensed under the MIT License. See the [LICENSE](https://github.com/BabaChain/BabaChain/blob/main/COPYING) file for details.

---

**Built with ❤️ by the BabaChain Team**