#!/bin/bash
# BabaChain Testnet Management Script
# Gerçek BabaChain binary'leri ile testnet yönetimi

set -e

# Renkler
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

CONTAINER_NAME="babachain-testnet"
COMPOSE_FILE="docker/docker-compose.testnet.yml"

# RPC bilgileri
RPC_USER="babachain_testnet"
RPC_PASSWORD=""

get_rpc_password() {
    if [ -f "docker/config/babachain-testnet.conf" ]; then
        RPC_PASSWORD=$(grep rpcpassword docker/config/babachain-testnet.conf 2>/dev/null | cut -d'=' -f2 || echo "")
    fi
}

show_help() {
    echo -e "${BLUE}🌐 BabaChain Testnet Manager${NC}"
    echo "Usage: $0 [COMMAND]"
    echo ""
    echo "Container Management:"
    echo "  start           - Start testnet node"
    echo "  stop            - Stop testnet node"
    echo "  restart         - Restart testnet node"
    echo "  status          - Show container status"
    echo "  logs            - Show logs"
    echo "  shell           - Enter container shell"
    echo ""
    echo "Blockchain Operations:"
    echo "  info            - Get blockchain info"
    echo "  staking         - Get staking info"
    echo "  peers           - Show peer connections"
    echo "  sync            - Check sync status"
    echo ""
    echo "Wallet Operations:"
    echo "  wallet          - Show wallet info"
    echo "  balance         - Show wallet balance"
    echo "  newaddress      - Generate new address"
    echo "  listaddresses   - List all addresses"
    echo "  backup          - Backup wallet"
    echo ""
    echo "Coin Generation:"
    echo "  generate [n]    - Generate n blocks (default: 1)"
    echo "  mine-start      - Start mining mode"
    echo "  mine-stop       - Stop mining"
    echo ""
    echo "Transaction Operations:"
    echo "  send <address> <amount>  - Send coins to address"
    echo "  listtx          - List transactions"
    echo "  gettx <txid>    - Get transaction details"
    echo ""
    echo "Network Operations:"
    echo "  addnode <ip:port>  - Add peer node"
    echo "  getpeerinfo     - Get detailed peer info"
    echo ""
    echo "Utility:"
    echo "  clean           - Clean all data (DANGEROUS!)"
    echo "  rebuild         - Rebuild and restart"
    echo "  help            - Show this help"
}

check_container() {
    if ! docker ps -a | grep -q ${CONTAINER_NAME}; then
        echo -e "${RED}❌ Testnet container bulunamadı! Önce başlatın.${NC}"
        exit 1
    fi
}

rpc_call() {
    local method=$1
    shift
    local params="$@"
    
    get_rpc_password
    if [ -z "$RPC_PASSWORD" ]; then
        echo -e "${RED}❌ RPC şifresi bulunamadı!${NC}"
        return 1
    fi
    
    docker exec ${CONTAINER_NAME} babachain-cli \
        -testnet \
        -rpcuser=${RPC_USER} \
        -rpcpassword=${RPC_PASSWORD} \
        ${method} ${params}
}

case "$1" in
    start)
        echo -e "${YELLOW}🚀 Testnet başlatılıyor...${NC}"
        if [ ! -f "src/babachaind" ]; then
            echo -e "${RED}❌ BabaChain binary'leri bulunamadı!${NC}"
            echo "Önce 'make' komutu ile build edin."
            exit 1
        fi
        docker-compose -f ${COMPOSE_FILE} up -d
        echo -e "${GREEN}✅ Testnet başlatıldı${NC}"
        ;;
        
    stop)
        echo -e "${YELLOW}⏹️  Testnet durduruluyor...${NC}"
        docker-compose -f ${COMPOSE_FILE} down
        echo -e "${GREEN}✅ Testnet durduruldu${NC}"
        ;;
        
    restart)
        echo -e "${YELLOW}🔄 Testnet yeniden başlatılıyor...${NC}"
        docker-compose -f ${COMPOSE_FILE} restart
        echo -e "${GREEN}✅ Testnet yeniden başlatıldı${NC}"
        ;;
        
    status)
        echo -e "${BLUE}📊 Testnet Durumu:${NC}"
        docker ps -a | grep ${CONTAINER_NAME} || echo "Container bulunamadı"
        echo ""
        if docker ps | grep -q ${CONTAINER_NAME}; then
            echo -e "${BLUE}📈 Kaynak Kullanımı:${NC}"
            docker stats ${CONTAINER_NAME} --no-stream 2>/dev/null
        fi
        ;;
        
    logs)
        echo -e "${BLUE}📋 Testnet Logları:${NC}"
        docker logs -f ${CONTAINER_NAME}
        ;;
        
    shell)
        check_container
        echo -e "${BLUE}🐚 Testnet container'a bağlanılıyor...${NC}"
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
        rpc_call getconnectioncount
        echo ""
        rpc_call getpeerinfo | head -20
        ;;
        
    sync)
        check_container
        echo -e "${BLUE}🔄 Sync Durumu:${NC}"
        rpc_call getblockchaininfo | grep -E "(blocks|headers|verificationprogress)"
        ;;
        
    wallet)
        check_container
        echo -e "${BLUE}💰 Wallet Bilgileri:${NC}"
        rpc_call getwalletinfo
        ;;
        
    balance)
        check_container
        echo -e "${BLUE}💳 Wallet Balance:${NC}"
        rpc_call getbalance
        ;;
        
    newaddress)
        check_container
        echo -e "${BLUE}🆕 Yeni Adres Oluşturuluyor:${NC}"
        NEW_ADDRESS=$(rpc_call getnewaddress)
        echo "Yeni adres: $NEW_ADDRESS"
        ;;
        
    listaddresses)
        check_container
        echo -e "${BLUE}📋 Tüm Adresler:${NC}"
        rpc_call listreceivedbyaddress 0 true
        ;;
        
    backup)
        check_container
        echo -e "${YELLOW}💾 Wallet backup oluşturuluyor...${NC}"
        BACKUP_FILE="wallet_testnet_backup_$(date +%Y%m%d_%H%M%S).dat"
        rpc_call backupwallet "/home/babachain/.babachain/${BACKUP_FILE}"
        docker cp ${CONTAINER_NAME}:/home/babachain/.babachain/${BACKUP_FILE} ./
        echo -e "${GREEN}✅ Backup oluşturuldu: ${BACKUP_FILE}${NC}"
        ;;
        
    generate)
        check_container
        BLOCKS=${2:-1}
        echo -e "${YELLOW}⛏️  $BLOCKS blok üretiliyor...${NC}"
        rpc_call generate $BLOCKS
        echo -e "${GREEN}✅ $BLOCKS blok üretildi${NC}"
        ;;
        
    mine-start)
        check_container
        echo -e "${YELLOW}⛏️  Mining başlatılıyor...${NC}"
        rpc_call setgenerate true 1
        echo -e "${GREEN}✅ Mining başlatıldı${NC}"
        ;;
        
    mine-stop)
        check_container
        echo -e "${YELLOW}⏹️  Mining durduruluyor...${NC}"
        rpc_call setgenerate false
        echo -e "${GREEN}✅ Mining durduruldu${NC}"
        ;;
        
    send)
        check_container
        if [ -z "$2" ] || [ -z "$3" ]; then
            echo -e "${RED}❌ Kullanım: $0 send <address> <amount>${NC}"
            exit 1
        fi
        ADDRESS=$2
        AMOUNT=$3
        echo -e "${YELLOW}💸 $AMOUNT coin $ADDRESS adresine gönderiliyor...${NC}"
        TXID=$(rpc_call sendtoaddress $ADDRESS $AMOUNT)
        echo -e "${GREEN}✅ Transaction gönderildi: $TXID${NC}"
        ;;
        
    listtx)
        check_container
        echo -e "${BLUE}📋 Son Transactionlar:${NC}"
        rpc_call listtransactions "" 10
        ;;
        
    gettx)
        check_container
        if [ -z "$2" ]; then
            echo -e "${RED}❌ Kullanım: $0 gettx <txid>${NC}"
            exit 1
        fi
        TXID=$2
        echo -e "${BLUE}🔍 Transaction Detayları:${NC}"
        rpc_call gettransaction $TXID
        ;;
        
    addnode)
        check_container
        if [ -z "$2" ]; then
            echo -e "${RED}❌ Kullanım: $0 addnode <ip:port>${NC}"
            exit 1
        fi
        NODE=$2
        echo -e "${YELLOW}🌐 Node ekleniyor: $NODE${NC}"
        rpc_call addnode $NODE "add"
        echo -e "${GREEN}✅ Node eklendi${NC}"
        ;;
        
    getpeerinfo)
        check_container
        echo -e "${BLUE}🌐 Detaylı Peer Bilgileri:${NC}"
        rpc_call getpeerinfo
        ;;
        
    clean)
        echo -e "${RED}⚠️  Bu işlem tüm testnet verisini silecek! Devam etmek istiyor musunuz? (y/N)${NC}"
        read -r response
        if [[ "$response" =~ ^([yY][eE][sS]|[yY])$ ]]; then
            echo -e "${YELLOW}🧹 Testnet verisi temizleniyor...${NC}"
            docker-compose -f ${COMPOSE_FILE} down -v
            docker rmi $(docker images | grep testnet | awk '{print $3}') 2>/dev/null || true
            echo -e "${GREEN}✅ Temizlik tamamlandı${NC}"
        else
            echo -e "${BLUE}İşlem iptal edildi${NC}"
        fi
        ;;
        
    rebuild)
        echo -e "${YELLOW}🏗️  Testnet yeniden build ediliyor...${NC}"
        docker-compose -f ${COMPOSE_FILE} down
        docker-compose -f ${COMPOSE_FILE} build --no-cache
        docker-compose -f ${COMPOSE_FILE} up -d
        echo -e "${GREEN}✅ Rebuild tamamlandı${NC}"
        ;;
        
    help|*)
        show_help
        ;;
esac