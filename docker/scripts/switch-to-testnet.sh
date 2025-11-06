#!/bin/bash
# BabaChain Testnet'e geçiş scripti

set -e

# Renkler
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🌐 BabaChain Testnet'e Geçiş${NC}"
echo "================================"

# Mevcut container'ı durdur
echo -e "${YELLOW}⏹️  Mevcut container durduruluyor...${NC}"
./docker/scripts/manage.sh stop 2>/dev/null || true

# Testnet konfigürasyonu oluştur
echo -e "${YELLOW}📝 Testnet konfigürasyonu hazırlanıyor...${NC}"

mkdir -p docker/config

cat > docker/config/babachain-testnet.conf << EOF
# BabaChain Testnet Configuration
# Otomatik oluşturuldu: $(date)

# Network
testnet=1
port=19999
rpcport=19998

# RPC ayarları
rpcuser=babachain_testnet
rpcpassword=testnet_$(openssl rand -hex 8)
rpcbind=0.0.0.0
rpcallowip=127.0.0.1
rpcallowip=172.16.0.0/12

# PoS staking ayarları
staking=1
stakegen=1
reservebalance=0

# Testnet seed nodes (örnek - gerçek testnet node'ları gerekli)
addnode=testnet-seed1.babachain.org:19999
addnode=testnet-seed2.babachain.org:19999
addnode=testnet-node1.babachain.org:19999

# Performans ayarları
maxconnections=50
timeout=5000
dbcache=200
maxmempool=100

# Debug
debug=pos,staking,net
printtoconsole=1

# Testnet specific
[test]
rpcport=19998
port=19999
EOF

# Testnet docker-compose dosyası oluştur
cat > docker/docker-compose.testnet.yml << EOF
services:
  babachain-testnet:
    build:
      context: ..
      dockerfile: docker/Dockerfile
    container_name: babachain-testnet
    restart: unless-stopped
    ports:
      - "19999:19999"  # P2P port
      - "127.0.0.1:19998:19998"  # RPC port
    volumes:
      - babachain-testnet-data:/home/babachain/.babachain
      - ./config/babachain-testnet.conf:/home/babachain/.babachain/babachain.conf:ro
      - ./logs:/home/babachain/logs
    environment:
      - TESTNET=1
      - RPC_USER=babachain_testnet
      - RPC_PASSWORD=\$(grep rpcpassword ./config/babachain-testnet.conf | cut -d'=' -f2)
      - STAKING=1
      - STAKEGEN=1
      - MAX_CONNECTIONS=50
      - DB_CACHE=200
      - DEBUG=pos,staking,net
    healthcheck:
      test: ["CMD", "babachain-cli", "-testnet", "getblockcount"]
      interval: 30s
      timeout: 10s
      retries: 3
      start_period: 120s
    networks:
      - babachain-testnet-network

volumes:
  babachain-testnet-data:
    driver: local

networks:
  babachain-testnet-network:
    driver: bridge
EOF

echo -e "${GREEN}✅ Testnet konfigürasyonu hazır${NC}"

echo -e "${YELLOW}⚠️  DİKKAT: Gerçek testnet için aşağıdakiler gerekli:${NC}"
echo "1. Gerçek BabaChain binary'leri (şu anda mock kullanıyoruz)"
echo "2. Çalışan testnet seed node'ları"
echo "3. Testnet blockchain verisi"
echo ""

echo -e "${BLUE}Testnet'i başlatmak için:${NC}"
echo "docker-compose -f docker/docker-compose.testnet.yml up -d"
echo ""

echo -e "${BLUE}Mock development'a geri dönmek için:${NC}"
echo "./docker/scripts/local-deploy.sh"