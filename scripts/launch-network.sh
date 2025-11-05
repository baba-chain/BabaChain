#!/bin/bash

# BabaChain Network Launch Script
# Bu script BabaChain ağının koordineli lansmanını gerçekleştirir

set -e

# Renkler
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

# Fonksiyonlar
print_status() {
    echo -e "${BLUE}[BİLGİ]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[BAŞARILI]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[UYARI]${NC} $1"
}

print_error() {
    echo -e "${RED}[HATA]${NC} $1"
}

print_header() {
    echo -e "${PURPLE}$1${NC}"
}

# Lansman türünü belirle
LAUNCH_TYPE=${1:-"mainnet"}
CURRENT_TIME=$(date +%s)

# Mainnet lansman zamanı (1 Aralık 2025, 00:00:00 UTC)
MAINNET_LAUNCH_TIME=1764547200

# Testnet lansman zamanı (1 Kasım 2025, 00:00:00 UTC)  
TESTNET_LAUNCH_TIME=1761955200

print_header "🚀 BabaChain Network Launch Script"
print_status "Lansman türü: $LAUNCH_TYPE"
print_status "Mevcut zaman: $(date -d @$CURRENT_TIME)"

# Lansman zamanını kontrol et
if [ "$LAUNCH_TYPE" = "mainnet" ]; then
    LAUNCH_TIME=$MAINNET_LAUNCH_TIME
    NETWORK_NAME="Mainnet"
    NETWORK_PORT=9999
    RPC_PORT=9998
elif [ "$LAUNCH_TYPE" = "testnet" ]; then
    LAUNCH_TIME=$TESTNET_LAUNCH_TIME
    NETWORK_NAME="Testnet"
    NETWORK_PORT=19999
    RPC_PORT=19998
else
    print_error "Geçersiz lansman türü. 'mainnet' veya 'testnet' kullanın."
    exit 1
fi

print_status "$NETWORK_NAME lansman zamanı: $(date -d @$LAUNCH_TIME)"

# Zaman kontrolü
TIME_DIFF=$((LAUNCH_TIME - CURRENT_TIME))
if [ $TIME_DIFF -gt 3600 ]; then
    print_warning "Lansman zamanına daha $((TIME_DIFF / 3600)) saat var."
    read -p "Yine de devam etmek istiyor musunuz? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
elif [ $TIME_DIFF -lt -3600 ]; then
    print_warning "Lansman zamanı $(((-TIME_DIFF) / 3600)) saat önce geçti."
    read -p "Yine de devam etmek istiyor musunuz? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Ön kontroller
print_header "🔍 Ön Kontroller"

# 1. BabaChain binary'lerinin varlığını kontrol et
print_status "BabaChain binary'leri kontrol ediliyor..."
if ! command -v babachaind &> /dev/null; then
    print_error "babachaind bulunamadı. Lütfen önce BabaChain'i derleyin."
    exit 1
fi

if ! command -v babachain-cli &> /dev/null; then
    print_error "babachain-cli bulunamadı. Lütfen önce BabaChain'i derleyin."
    exit 1
fi

print_success "BabaChain binary'leri mevcut"

# 2. Konfigürasyon dosyasını kontrol et
print_status "Konfigürasyon dosyası kontrol ediliyor..."
if [ "$LAUNCH_TYPE" = "mainnet" ]; then
    CONFIG_FILE="$HOME/.babachain/babachain.conf"
else
    CONFIG_FILE="$HOME/.babachain/babachain.conf"
fi

if [ ! -f "$CONFIG_FILE" ]; then
    print_error "Konfigürasyon dosyası bulunamadı: $CONFIG_FILE"
    exit 1
fi

print_success "Konfigürasyon dosyası mevcut"

# 3. Seed node'ları kontrol et
print_status "Seed node'lar kontrol ediliyor..."
SEED_NODES=("seed1.babachain.org" "seed2.babachain.org" "seed3.babachain.org")
AVAILABLE_SEEDS=0

for seed in "${SEED_NODES[@]}"; do
    if nc -z -w5 "$seed" $NETWORK_PORT 2>/dev/null; then
        print_success "✅ $seed:$NETWORK_PORT erişilebilir"
        ((AVAILABLE_SEEDS++))
    else
        print_warning "❌ $seed:$NETWORK_PORT erişilemiyor"
    fi
done

if [ $AVAILABLE_SEEDS -eq 0 ]; then
    print_error "Hiçbir seed node erişilebilir değil!"
    exit 1
elif [ $AVAILABLE_SEEDS -lt 3 ]; then
    print_warning "Sadece $AVAILABLE_SEEDS seed node erişilebilir"
    read -p "Devam etmek istiyor musunuz? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# 4. Port kontrolü
print_status "Port kullanımı kontrol ediliyor..."
if netstat -tuln | grep -q ":$NETWORK_PORT "; then
    print_error "Port $NETWORK_PORT zaten kullanımda!"
    exit 1
fi

if netstat -tuln | grep -q ":$RPC_PORT "; then
    print_error "Port $RPC_PORT zaten kullanımda!"
    exit 1
fi

print_success "Portlar müsait"

# Lansman onayı
print_header "🎯 Lansman Onayı"
print_status "Lansman parametreleri:"
echo "  Network: $NETWORK_NAME"
echo "  P2P Port: $NETWORK_PORT"
echo "  RPC Port: $RPC_PORT"
echo "  Lansman zamanı: $(date -d @$LAUNCH_TIME)"
echo "  Mevcut zaman: $(date -d @$CURRENT_TIME)"

read -p "Lansmanı başlatmak istiyor musunuz? (y/N): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    print_warning "Lansman iptal edildi."
    exit 1
fi

# Lansman geri sayımı
print_header "⏰ Lansman Geri Sayımı"
if [ $TIME_DIFF -gt 0 ] && [ $TIME_DIFF -lt 300 ]; then
    print_status "Lansman zamanına kadar geri sayım başlıyor..."
    while [ $TIME_DIFF -gt 0 ]; do
        printf "\r${YELLOW}Lansmana kalan süre: %02d:%02d${NC}" $((TIME_DIFF/60)) $((TIME_DIFF%60))
        sleep 1
        TIME_DIFF=$((TIME_DIFF-1))
    done
    echo
fi

# BabaChain'i başlat
print_header "🚀 BabaChain Başlatılıyor"

# Daemon'u başlat
print_status "BabaChain daemon'u başlatılıyor..."
if [ "$LAUNCH_TYPE" = "testnet" ]; then
    babachaind -testnet -daemon
else
    babachaind -daemon
fi

# Başlatma kontrolü
print_status "Daemon başlatma kontrolü..."
sleep 10

# RPC bağlantısını test et
MAX_RETRIES=30
RETRY_COUNT=0

while [ $RETRY_COUNT -lt $MAX_RETRIES ]; do
    if [ "$LAUNCH_TYPE" = "testnet" ]; then
        if babachain-cli -testnet getblockchaininfo &>/dev/null; then
            break
        fi
    else
        if babachain-cli getblockchaininfo &>/dev/null; then
            break
        fi
    fi
    
    print_status "RPC bağlantısı bekleniyor... ($((RETRY_COUNT+1))/$MAX_RETRIES)"
    sleep 2
    ((RETRY_COUNT++))
done

if [ $RETRY_COUNT -eq $MAX_RETRIES ]; then
    print_error "RPC bağlantısı kurulamadı!"
    exit 1
fi

print_success "BabaChain daemon başarıyla başlatıldı!"

# Genesis block kontrolü
print_header "🎯 Genesis Block Kontrolü"
print_status "Genesis block kontrol ediliyor..."

if [ "$LAUNCH_TYPE" = "testnet" ]; then
    GENESIS_HASH=$(babachain-cli -testnet getblockhash 0)
    BLOCK_INFO=$(babachain-cli -testnet getblock "$GENESIS_HASH")
else
    GENESIS_HASH=$(babachain-cli getblockhash 0)
    BLOCK_INFO=$(babachain-cli getblock "$GENESIS_HASH")
fi

print_success "Genesis block hash: $GENESIS_HASH"

# Genesis block detaylarını göster
echo "$BLOCK_INFO" | jq '{
    hash: .hash,
    time: .time,
    nTx: .nTx,
    size: .size,
    merkleroot: .merkleroot
}'

# Network durumunu kontrol et
print_header "🌐 Network Durumu"
print_status "Network bilgileri alınıyor..."

if [ "$LAUNCH_TYPE" = "testnet" ]; then
    NETWORK_INFO=$(babachain-cli -testnet getnetworkinfo)
    BLOCKCHAIN_INFO=$(babachain-cli -testnet getblockchaininfo)
else
    NETWORK_INFO=$(babachain-cli getnetworkinfo)
    BLOCKCHAIN_INFO=$(babachain-cli getblockchaininfo)
fi

echo "Network Bilgileri:"
echo "$NETWORK_INFO" | jq '{
    version: .version,
    subversion: .subversion,
    connections: .connections,
    networkactive: .networkactive,
    localaddresses: .localaddresses
}'

echo "Blockchain Bilgileri:"
echo "$BLOCKCHAIN_INFO" | jq '{
    chain: .chain,
    blocks: .blocks,
    headers: .headers,
    bestblockhash: .bestblockhash,
    difficulty: .difficulty,
    verificationprogress: .verificationprogress
}'

# Peer bağlantılarını kontrol et
print_status "Peer bağlantıları kontrol ediliyor..."
sleep 30  # Peer'ların bağlanması için bekle

if [ "$LAUNCH_TYPE" = "testnet" ]; then
    PEER_COUNT=$(babachain-cli -testnet getconnectioncount)
    PEER_INFO=$(babachain-cli -testnet getpeerinfo)
else
    PEER_COUNT=$(babachain-cli getconnectioncount)
    PEER_INFO=$(babachain-cli getpeerinfo)
fi

print_status "Bağlı peer sayısı: $PEER_COUNT"

if [ "$PEER_COUNT" -gt 0 ]; then
    print_success "Peer bağlantıları kuruldu!"
    echo "$PEER_INFO" | jq '.[] | {addr: .addr, version: .version, subver: .subver}'
else
    print_warning "Henüz peer bağlantısı yok"
fi

# Staking durumunu kontrol et
print_header "🥩 Staking Durumu"
print_status "Staking durumu kontrol ediliyor..."

if [ "$LAUNCH_TYPE" = "testnet" ]; then
    STAKING_INFO=$(babachain-cli -testnet getstakinginfo 2>/dev/null || echo '{"enabled": false, "error": "No wallet"}')
else
    STAKING_INFO=$(babachain-cli getstakinginfo 2>/dev/null || echo '{"enabled": false, "error": "No wallet"}')
fi

echo "Staking Bilgileri:"
echo "$STAKING_INFO" | jq '{
    enabled: .enabled,
    staking: .staking,
    errors: .errors,
    currentblocksize: .currentblocksize,
    currentblocktx: .currentblocktx,
    difficulty: .difficulty
}'

# Lansman özeti
print_header "🎉 Lansman Özeti"
print_success "$NETWORK_NAME başarıyla başlatıldı!"

echo "═══════════════════════════════════════════════════════════════"
echo "                    LANSMAN ÖZETİ"
echo "═══════════════════════════════════════════════════════════════"
echo "🌐 Network: $NETWORK_NAME"
echo "🔗 Genesis Hash: $GENESIS_HASH"
echo "👥 Peer Sayısı: $PEER_COUNT"
echo "⏰ Lansman Zamanı: $(date -d @$LAUNCH_TIME)"
echo "📊 Block Sayısı: $(echo "$BLOCKCHAIN_INFO" | jq -r '.blocks')"
echo "🔧 RPC Port: $RPC_PORT"
echo "🌍 P2P Port: $NETWORK_PORT"
echo "═══════════════════════════════════════════════════════════════"

# Sonraki adımlar
print_header "📋 Sonraki Adımlar"
echo "1. Network sağlığını izleyin:"
if [ "$LAUNCH_TYPE" = "testnet" ]; then
    echo "   babachain-cli -testnet getblockchaininfo"
    echo "   babachain-cli -testnet getnetworkinfo"
    echo "   babachain-cli -testnet getpeerinfo"
else
    echo "   babachain-cli getblockchaininfo"
    echo "   babachain-cli getnetworkinfo"
    echo "   babachain-cli getpeerinfo"
fi

echo "2. Cüzdan oluşturun ve staking'e başlayın"
echo "3. Community'yi bilgilendirin"
echo "4. Exchange'lere entegrasyon başlatın"
echo "5. Marketing kampanyalarını aktif edin"

# Log izleme önerisi
print_status "Logları izlemek için:"
echo "tail -f ~/.babachain/debug.log"

print_success "🎊 BabaChain $NETWORK_NAME lansmanı tamamlandı!"