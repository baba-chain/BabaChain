#!/bin/bash
# BabaChain Docker Management Script
# Bu script BabaChain Docker container'ını yönetmenizi sağlar

set -e

# Renkler
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

CONTAINER_NAME="babachain-local"
COMPOSE_FILE="docker/docker-compose.local.yml"

# RPC bilgileri
RPC_USER="babachain_local"
RPC_PASSWORD=$(grep rpcpassword docker/config/babachain.conf 2>/dev/null | cut -d'=' -f2 || echo "")

show_help() {
    echo -e "${BLUE}🐳 BabaChain Docker Management${NC}"
    echo "Usage: $0 [COMMAND]"
    echo ""
    echo "Commands:"
    echo "  start       - Container'ı başlat"
    echo "  stop        - Container'ı durdur"
    echo "  restart     - Container'ı yeniden başlat"
    echo "  status      - Container durumunu göster"
    echo "  logs        - Logları göster"
    echo "  shell       - Container'a bash ile gir"
    echo "  info        - Blockchain bilgilerini göster"
    echo "  staking     - Staking bilgilerini göster"
    echo "  peers       - Peer bağlantılarını göster"
    echo "  wallet      - Wallet bilgilerini göster"
    echo "  backup      - Wallet backup oluştur"
    echo "  clean       - Container ve volume'ları temizle"
    echo "  rebuild     - Image'ı yeniden build et ve başlat"
    echo "  monitor     - Canlı monitoring"
    echo "  help        - Bu yardım mesajını göster"
}

check_container() {
    if ! docker ps -a | grep -q ${CONTAINER_NAME}; then
        echo -e "${RED}❌ Container bulunamadı! Önce deploy edin.${NC}"
        exit 1
    fi
}

rpc_call() {
    local method=$1
    shift
    local params="$@"
    
    if [ -z "$RPC_PASSWORD" ]; then
        echo -e "${RED}❌ RPC şifresi bulunamadı!${NC}"
        return 1
    fi
    
    docker exec ${CONTAINER_NAME} babachain-cli \
        -rpcuser=${RPC_USER} \
        -rpcpassword=${RPC_PASSWORD} \
        ${method} ${params}
}

case "$1" in
    start)
        echo -e "${YELLOW}🚀 Container başlatılıyor...${NC}"
        docker-compose -f ${COMPOSE_FILE} up -d
        echo -e "${GREEN}✅ Container başlatıldı${NC}"
        ;;
        
    stop)
        echo -e "${YELLOW}⏹️  Container durduruluyor...${NC}"
        docker-compose -f ${COMPOSE_FILE} down
        echo -e "${GREEN}✅ Container durduruldu${NC}"
        ;;
        
    restart)
        echo -e "${YELLOW}🔄 Container yeniden başlatılıyor...${NC}"
        docker-compose -f ${COMPOSE_FILE} restart
        echo -e "${GREEN}✅ Container yeniden başlatıldı${NC}"
        ;;
        
    status)
        echo -e "${BLUE}📊 Container Durumu:${NC}"
        docker ps -a | grep ${CONTAINER_NAME} || echo "Container bulunamadı"
        echo ""
        echo -e "${BLUE}📈 Kaynak Kullanımı:${NC}"
        docker stats ${CONTAINER_NAME} --no-stream 2>/dev/null || echo "Container çalışmıyor"
        ;;
        
    logs)
        echo -e "${BLUE}📋 Container Logları:${NC}"
        docker logs -f ${CONTAINER_NAME}
        ;;
        
    shell)
        check_container
        echo -e "${BLUE}🐚 Container shell'e bağlanılıyor...${NC}"
        docker exec -it ${CONTAINER_NAME} bash
        ;;
        
    info)
        check_container
        echo -e "${BLUE}ℹ️  Blockchain Bilgileri:${NC}"
        rpc_call getblockchaininfo
        ;;
        
    staking)
        check_container
        echo -e "${BLUE}🥩 Staking Bilgileri:${NC}"
        rpc_call getstakinginfo
        ;;
        
    peers)
        check_container
        echo -e "${BLUE}🌐 Peer Bağlantıları:${NC}"
        rpc_call getpeerinfo | head -20
        ;;
        
    wallet)
        check_container
        echo -e "${BLUE}💰 Wallet Bilgileri:${NC}"
        rpc_call getwalletinfo
        echo ""
        echo -e "${BLUE}💳 Balance:${NC}"
        rpc_call getbalance
        ;;
        
    backup)
        check_container
        echo -e "${YELLOW}💾 Wallet backup oluşturuluyor...${NC}"
        BACKUP_FILE="wallet_backup_$(date +%Y%m%d_%H%M%S).dat"
        rpc_call backupwallet "/home/babachain/.babachain/${BACKUP_FILE}"
        docker cp ${CONTAINER_NAME}:/home/babachain/.babachain/${BACKUP_FILE} ./
        echo -e "${GREEN}✅ Backup oluşturuldu: ${BACKUP_FILE}${NC}"
        ;;
        
    clean)
        echo -e "${RED}⚠️  Bu işlem tüm veriyi silecek! Devam etmek istiyor musunuz? (y/N)${NC}"
        read -r response
        if [[ "$response" =~ ^([yY][eE][sS]|[yY])$ ]]; then
            echo -e "${YELLOW}🧹 Temizlik yapılıyor...${NC}"
            docker-compose -f ${COMPOSE_FILE} down -v
            docker rmi $(docker images | grep babachain | awk '{print $3}') 2>/dev/null || true
            echo -e "${GREEN}✅ Temizlik tamamlandı${NC}"
        else
            echo -e "${BLUE}İşlem iptal edildi${NC}"
        fi
        ;;
        
    rebuild)
        echo -e "${YELLOW}🏗️  Image yeniden build ediliyor...${NC}"
        docker-compose -f ${COMPOSE_FILE} down
        docker-compose -f ${COMPOSE_FILE} build --no-cache
        docker-compose -f ${COMPOSE_FILE} up -d
        echo -e "${GREEN}✅ Rebuild tamamlandı${NC}"
        ;;
        
    monitor)
        check_container
        echo -e "${BLUE}📊 Canlı Monitoring (Çıkmak için Ctrl+C)${NC}"
        echo ""
        
        while true; do
            clear
            echo -e "${BLUE}=== BabaChain Node Monitor ===${NC}"
            echo "Zaman: $(date)"
            echo ""
            
            echo -e "${YELLOW}Container Durumu:${NC}"
            docker ps | grep ${CONTAINER_NAME} | awk '{print $2 " - " $7 " - " $8}'
            echo ""
            
            echo -e "${YELLOW}Blockchain Info:${NC}"
            rpc_call getblockchaininfo | grep -E "(blocks|headers|verificationprogress)" || echo "RPC bağlantısı yok"
            echo ""
            
            echo -e "${YELLOW}Peer Sayısı:${NC}"
            PEER_COUNT=$(rpc_call getconnectioncount 2>/dev/null || echo "0")
            echo "Bağlı peer sayısı: ${PEER_COUNT}"
            echo ""
            
            echo -e "${YELLOW}Staking Durumu:${NC}"
            rpc_call getstakinginfo | grep -E "(enabled|staking|weight)" || echo "Staking bilgisi alınamadı"
            echo ""
            
            echo -e "${YELLOW}Kaynak Kullanımı:${NC}"
            docker stats ${CONTAINER_NAME} --no-stream --format "table {{.CPUPerc}}\t{{.MemUsage}}\t{{.NetIO}}\t{{.BlockIO}}"
            
            sleep 5
        done
        ;;
        
    help|*)
        show_help
        ;;
esac