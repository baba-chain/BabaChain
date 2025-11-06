#!/bin/bash
# BabaChain Local Docker Deployment Script
# Bu script BabaChain'i local Docker Desktop ortamında kolayca deploy etmenizi sağlar

set -e

# Renkler
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Konfigürasyon
COMPOSE_FILE="docker/docker-compose.yml"
PROJECT_NAME="babachain-local"
CONTAINER_NAME="babachain-local"

echo -e "${BLUE}🐳 BabaChain Local Docker Deployment${NC}"
echo "=================================="

# Docker kontrolü
if ! command -v docker &> /dev/null; then
    echo -e "${RED}❌ Docker bulunamadı! Lütfen Docker Desktop'ı kurun.${NC}"
    exit 1
fi

if ! docker info &> /dev/null; then
    echo -e "${RED}❌ Docker çalışmıyor! Lütfen Docker Desktop'ı başlatın.${NC}"
    exit 1
fi

echo -e "${GREEN}✅ Docker hazır${NC}"

# Proje dizini kontrolü
if [ ! -f "docker/Dockerfile" ]; then
    echo -e "${RED}❌ BabaChain proje dizininde değilsiniz!${NC}"
    echo "Lütfen BabaChain ana dizinine gidin ve scripti tekrar çalıştırın."
    exit 1
fi

echo -e "${GREEN}✅ Proje dizini doğru${NC}"

# Konfigürasyon dosyası oluşturma
echo -e "${YELLOW}📝 Konfigürasyon dosyası hazırlanıyor...${NC}"

mkdir -p docker/config

cat > docker/config/babachain.conf << EOF
# BabaChain Local Development Configuration
# Otomatik oluşturuldu: $(date)

# RPC ayarları
rpcuser=babachain_local
rpcpassword=local_dev_$(openssl rand -hex 8)
rpcport=9998
rpcbind=0.0.0.0
rpcallowip=127.0.0.1
rpcallowip=172.16.0.0/12

# Ağ ayarları
port=9999
listen=1
discover=1
upnp=0

# PoS staking ayarları
staking=1
stakegen=1
reservebalance=0

# Seed nodes
addnode=seed1.babachain.org:9999
addnode=seed2.babachain.org:9999
addnode=seed3.babachain.org:9999

# Performans ayarları (local development)
maxconnections=50
timeout=5000
dbcache=200
maxmempool=100

# Debug
debug=pos,staking,net
printtoconsole=1

# Network
testnet=0
regtest=0
EOF

echo -e "${GREEN}✅ Konfigürasyon dosyası oluşturuldu${NC}"

# Local docker-compose dosyası oluşturma
echo -e "${YELLOW}📝 Docker Compose konfigürasyonu hazırlanıyor...${NC}"

cat > docker/docker-compose.local.yml << EOF
services:
  babachain:
    build:
      context: ..
      dockerfile: docker/Dockerfile.minimal
    container_name: ${CONTAINER_NAME}
    restart: unless-stopped
    ports:
      - "19999:9999"  # P2P port
      - "127.0.0.1:19998:9998"  # RPC port (localhost only)
    volumes:
      - babachain-local-data:/home/babachain/.babachain
      - ./logs:/home/babachain/logs
    environment:
      - RPC_USER=babachain_local
      - RPC_PASSWORD=\$(grep rpcpassword ./config/babachain.conf | cut -d'=' -f2)
      - STAKING=1
      - STAKEGEN=1
      - MAX_CONNECTIONS=50
      - DB_CACHE=200
      - DEBUG=pos,staking,net
    healthcheck:
      test: ["CMD", "echo", "healthy"]
      interval: 30s
      timeout: 10s
      retries: 3
      start_period: 30s
    networks:
      - babachain-network

volumes:
  babachain-local-data:
    driver: local

networks:
  babachain-network:
    driver: bridge
EOF

echo -e "${GREEN}✅ Docker Compose konfigürasyonu hazır${NC}"

# Log dizini oluşturma
mkdir -p docker/logs

# Mevcut container kontrolü
if docker ps -a | grep -q ${CONTAINER_NAME}; then
    echo -e "${YELLOW}⚠️  Mevcut container bulundu. Kaldırılıyor...${NC}"
    docker stop ${CONTAINER_NAME} 2>/dev/null || true
    docker rm ${CONTAINER_NAME} 2>/dev/null || true
fi

# Build ve başlatma
echo -e "${YELLOW}🏗️  Docker image build ediliyor...${NC}"
docker-compose -f docker/docker-compose.local.yml build

echo -e "${YELLOW}🚀 Container başlatılıyor...${NC}"
docker-compose -f docker/docker-compose.local.yml up -d

# Başlatma kontrolü
echo -e "${YELLOW}⏳ Container'ın başlaması bekleniyor...${NC}"
sleep 10

if docker ps | grep -q ${CONTAINER_NAME}; then
    echo -e "${GREEN}✅ BabaChain başarıyla başlatıldı!${NC}"
    
    # Bağlantı bilgileri
    echo ""
    echo -e "${BLUE}📊 Bağlantı Bilgileri:${NC}"
    echo "Container Adı: ${CONTAINER_NAME}"
    echo "P2P Port: 19999"
    echo "RPC Port: 19998 (localhost only)"
    echo "RPC Kullanıcı: babachain_local"
    echo "RPC Şifre: $(grep rpcpassword docker/config/babachain.conf | cut -d'=' -f2)"
    
    echo ""
    echo -e "${BLUE}🔧 Yararlı Komutlar:${NC}"
    echo "Logları görüntüle: docker logs -f ${CONTAINER_NAME}"
    echo "Container'a gir: docker exec -it ${CONTAINER_NAME} bash"
    echo "Blockchain info: docker exec ${CONTAINER_NAME} babachain-cli getblockchaininfo"
    echo "Staking info: docker exec ${CONTAINER_NAME} babachain-cli getstakinginfo"
    echo "Durdur: docker-compose -f docker/docker-compose.local.yml down"
    
    echo ""
    echo -e "${GREEN}🎉 Deployment tamamlandı! Logları takip etmek için:${NC}"
    echo "docker logs -f ${CONTAINER_NAME}"
    
else
    echo -e "${RED}❌ Container başlatılamadı!${NC}"
    echo "Hata logları:"
    docker logs ${CONTAINER_NAME} 2>/dev/null || echo "Log bulunamadı"
    exit 1
fi