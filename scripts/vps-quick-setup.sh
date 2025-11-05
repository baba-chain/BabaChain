#!/bin/bash

# BabaChain VPS Quick Setup Script
# Bu script VPS'lerde hızlı BabaChain seed node kurulumu yapar

set -e

# Renkler
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

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

# Parametreler
NODE_TYPE=${1:-"seed"}  # seed, validator, regular
NODE_NAME=${2:-"node-$(hostname)"}
NETWORK=${3:-"mainnet"}  # mainnet, testnet

print_status "🚀 BabaChain VPS Quick Setup başlatılıyor..."
print_status "Node türü: $NODE_TYPE"
print_status "Node adı: $NODE_NAME"
print_status "Network: $NETWORK"

# Root kontrolü
if [[ $EUID -ne 0 ]]; then
   print_error "Bu script root olarak çalıştırılmalıdır!"
   print_status "Çalıştırın: sudo $0 $@"
   exit 1
fi

# İşletim sistemi kontrolü
if ! grep -q "Ubuntu" /etc/os-release; then
    print_warning "Bu script Ubuntu için optimize edilmiştir"
    read -p "Devam etmek istiyor musunuz? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Sistem bilgileri
print_status "Sistem bilgileri:"
echo "  OS: $(lsb_release -d | cut -f2)"
echo "  Kernel: $(uname -r)"
echo "  CPU: $(nproc) cores"
echo "  RAM: $(free -h | awk '/^Mem:/ {print $2}')"
echo "  Disk: $(df -h / | awk 'NR==2 {print $4}') free"
echo "  IP: $(curl -s ifconfig.me || echo "Unknown")"

# 1. Sistem güncelleme
print_status "📦 Sistem güncelleniyor..."
export DEBIAN_FRONTEND=noninteractive
apt update && apt upgrade -y

# 2. Gerekli paketleri yükle
print_status "🛠️ Gerekli paketler yükleniyor..."
apt install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    libboost-all-dev \
    libssl-dev \
    libdb++-dev \
    libminiupnpc-dev \
    libzmq3-dev \
    libqt5gui5 \
    libqt5core5a \
    libqt5dbus5 \
    qttools5-dev \
    qttools5-dev-tools \
    libprotobuf-dev \
    protobuf-compiler \
    pkg-config \
    libevent-dev \
    libqrencode-dev \
    htop \
    iotop \
    nethogs \
    ufw \
    fail2ban \
    logrotate

# 3. BabaChain kullanıcısı oluştur
print_status "👤 BabaChain kullanıcısı oluşturuluyor..."
if ! id "babachain" &>/dev/null; then
    adduser --disabled-password --gecos "" babachain
    usermod -aG sudo babachain
    print_success "BabaChain kullanıcısı oluşturuldu"
else
    print_warning "BabaChain kullanıcısı zaten mevcut"
fi

# 4. BabaChain kaynak kodunu indir
print_status "📥 BabaChain kaynak kodu indiriliyor..."
sudo -u babachain bash << 'EOF'
cd /home/babachain
if [ ! -d "BabaChain" ]; then
    git clone https://github.com/BabaChain/BabaChain.git
else
    cd BabaChain
    git pull origin main
    cd ..
fi
EOF

# 5. BabaChain'i derle
print_status "🔨 BabaChain derleniyor..."
sudo -u babachain bash << 'EOF'
cd /home/babachain/BabaChain
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
EOF

# Binary'leri yükle
cd /home/babachain/BabaChain/build
make install

print_success "BabaChain başarıyla derlendi ve yüklendi"

# 6. Konfigürasyon oluştur
print_status "⚙️ Konfigürasyon dosyaları oluşturuluyor..."

# Node türüne göre konfigürasyon
if [ "$NODE_TYPE" = "seed" ]; then
    MAX_CONNECTIONS=200
    LISTEN=1
    DISCOVER=1
    UPNP=1
elif [ "$NODE_TYPE" = "validator" ]; then
    MAX_CONNECTIONS=125
    LISTEN=1
    DISCOVER=1
    UPNP=0
else
    MAX_CONNECTIONS=50
    LISTEN=0
    DISCOVER=1
    UPNP=1
fi

# Network portları
if [ "$NETWORK" = "testnet" ]; then
    P2P_PORT=19999
    RPC_PORT=19998
    TESTNET=1
else
    P2P_PORT=9999
    RPC_PORT=9998
    TESTNET=0
fi

# RPC şifresi oluştur
RPC_PASSWORD=$(openssl rand -base64 32)

# Konfigürasyon dosyasını oluştur
sudo -u babachain mkdir -p /home/babachain/.babachain
sudo -u babachain tee /home/babachain/.babachain/babachain.conf > /dev/null << EOF
# BabaChain VPS Konfigürasyonu - $NODE_TYPE
# Node: $NODE_NAME
# Oluşturulma: $(date)

# RPC ayarları
rpcuser=babachain_${NODE_TYPE}
rpcpassword=$RPC_PASSWORD
rpcport=$RPC_PORT
rpcbind=0.0.0.0
rpcallowip=0.0.0.0/0

# Ağ ayarları
port=$P2P_PORT
listen=$LISTEN
discover=$DISCOVER
upnp=$UPNP
maxconnections=$MAX_CONNECTIONS

# PoS staking ayarları
staking=1
stakegen=1
reservebalance=100

# Seed node'lar (eğer bu bir seed node değilse)
EOF

# Seed node değilse, diğer seed node'ları ekle
if [ "$NODE_TYPE" != "seed" ]; then
    sudo -u babachain tee -a /home/babachain/.babachain/babachain.conf > /dev/null << EOF
addnode=seed1.babachain.org:$P2P_PORT
addnode=seed2.babachain.org:$P2P_PORT
addnode=seed3.babachain.org:$P2P_PORT
EOF
fi

# Konfigürasyonun devamı
sudo -u babachain tee -a /home/babachain/.babachain/babachain.conf > /dev/null << EOF

# Performans ayarları
timeout=5000
dbcache=512
maxmempool=500

# Güvenlik
rpcssl=0

# Loglama
debug=pos
debug=staking
debug=net

# Network
testnet=$TESTNET
regtest=0

# VPS optimizasyonları
uacomment=BabaChain-VPS-$NODE_TYPE-$NODE_NAME
EOF

chmod 600 /home/babachain/.babachain/babachain.conf
chown babachain:babachain /home/babachain/.babachain/babachain.conf

print_success "Konfigürasyon dosyası oluşturuldu"
print_warning "RPC şifresi: $RPC_PASSWORD"

# 7. Systemd servisi oluştur
print_status "🔧 Systemd servisi oluşturuluyor..."
tee /etc/systemd/system/babachaind.service > /dev/null << EOF
[Unit]
Description=BabaChain daemon ($NODE_TYPE node)
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

# Güvenlik ayarları
NoNewPrivileges=true
PrivateDevices=true
ProtectHome=true
ProtectSystem=strict
ReadWritePaths=/home/babachain/.babachain

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable babachaind

print_success "Systemd servisi oluşturuldu"

# 8. Firewall ayarları
print_status "🛡️ Firewall ayarları yapılandırılıyor..."
ufw --force enable
ufw allow ssh
ufw allow $P2P_PORT/tcp comment "BabaChain P2P"

# Seed node ise RPC'yi de aç
if [ "$NODE_TYPE" = "seed" ]; then
    ufw allow $RPC_PORT/tcp comment "BabaChain RPC"
fi

print_success "Firewall kuralları eklendi"

# 9. Fail2ban konfigürasyonu
print_status "🛡️ Fail2ban yapılandırılıyor..."
tee /etc/fail2ban/jail.d/babachain.conf > /dev/null << EOF
[babachain]
enabled = true
port = $P2P_PORT
filter = babachain
logpath = /home/babachain/.babachain/debug.log
maxretry = 5
bantime = 3600
findtime = 600
EOF

systemctl restart fail2ban

# 10. Monitoring scriptleri
print_status "📊 Monitoring scriptleri oluşturuluyor..."

# Sistem monitoring
sudo -u babachain tee /home/babachain/monitor.sh > /dev/null << 'EOF'
#!/bin/bash
LOG_FILE="/home/babachain/monitor.log"
DATE=$(date '+%Y-%m-%d %H:%M:%S')

# Sistem kaynakları
CPU_USAGE=$(top -bn1 | grep "Cpu(s)" | awk '{print $2}' | cut -d'%' -f1)
MEM_USAGE=$(free | grep Mem | awk '{printf("%.1f", $3/$2 * 100.0)}')
DISK_USAGE=$(df / | awk 'NR==2 {print $5}' | cut -d'%' -f1)

# BabaChain durumu
if pgrep -x "babachaind" > /dev/null; then
    DAEMON_STATUS="RUNNING"
    BLOCK_COUNT=$(babachain-cli getblockcount 2>/dev/null || echo "N/A")
    PEER_COUNT=$(babachain-cli getconnectioncount 2>/dev/null || echo "N/A")
    STAKING_STATUS=$(babachain-cli getstakinginfo 2>/dev/null | jq -r '.staking // "N/A"')
else
    DAEMON_STATUS="STOPPED"
    BLOCK_COUNT="N/A"
    PEER_COUNT="N/A"
    STAKING_STATUS="N/A"
fi

echo "$DATE - CPU:${CPU_USAGE}% MEM:${MEM_USAGE}% DISK:${DISK_USAGE}% DAEMON:$DAEMON_STATUS BLOCKS:$BLOCK_COUNT PEERS:$PEER_COUNT STAKING:$STAKING_STATUS" >> $LOG_FILE
EOF

chmod +x /home/babachain/monitor.sh

# Cron job ekle
sudo -u babachain bash << 'EOF'
(crontab -l 2>/dev/null; echo "*/5 * * * * /home/babachain/monitor.sh") | crontab -
EOF

# 11. Yedekleme scripti
print_status "💾 Yedekleme scripti oluşturuluyor..."
sudo -u babachain tee /home/babachain/backup.sh > /dev/null << 'EOF'
#!/bin/bash
BACKUP_DIR="/home/babachain/backups"
DATE=$(date +%Y%m%d_%H%M%S)

mkdir -p $BACKUP_DIR

# Cüzdan yedekle (eğer varsa)
if [ -f ~/.babachain/wallet.dat ]; then
    cp ~/.babachain/wallet.dat $BACKUP_DIR/wallet_$DATE.dat
fi

# Konfigürasyon yedekle
cp ~/.babachain/babachain.conf $BACKUP_DIR/babachain_conf_$DATE.conf

# Eski yedekleri temizle (7 günden eski)
find $BACKUP_DIR -name "wallet_*.dat" -mtime +7 -delete 2>/dev/null || true
find $BACKUP_DIR -name "babachain_conf_*.conf" -mtime +7 -delete 2>/dev/null || true

echo "$(date): Yedekleme tamamlandı"
EOF

chmod +x /home/babachain/backup.sh

# Günlük yedekleme
sudo -u babachain bash << 'EOF'
(crontab -l 2>/dev/null; echo "0 2 * * * /home/babachain/backup.sh >> /home/babachain/backup.log 2>&1") | crontab -
EOF

# 12. Kullanışlı aliaslar
print_status "🔧 Kullanışlı aliaslar oluşturuluyor..."
sudo -u babachain tee -a /home/babachain/.bashrc > /dev/null << 'EOF'

# BabaChain aliasları
alias baba-cli='babachain-cli'
alias baba-status='babachain-cli getblockchaininfo'
alias baba-balance='babachain-cli getbalance'
alias baba-staking='babachain-cli getstakinginfo'
alias baba-peers='babachain-cli getpeerinfo | jq ".[].addr"'
alias baba-logs='tail -f ~/.babachain/debug.log'
alias baba-start='sudo systemctl start babachaind'
alias baba-stop='sudo systemctl stop babachaind'
alias baba-restart='sudo systemctl restart babachaind'
alias baba-service='sudo systemctl status babachaind'
alias baba-monitor='tail -f ~/monitor.log'
EOF

# 13. BabaChain'i başlat
print_status "🚀 BabaChain servisi başlatılıyor..."
systemctl start babachaind

# Başlatma kontrolü
sleep 10
if systemctl is-active --quiet babachaind; then
    print_success "BabaChain servisi başarıyla başlatıldı!"
else
    print_error "BabaChain servisi başlatılamadı"
    print_status "Logları kontrol edin: journalctl -u babachaind -f"
fi

# 14. Kurulum özeti
print_success "🎉 BabaChain VPS kurulumu tamamlandı!"
echo
echo "═══════════════════════════════════════════════════════════════"
echo "                    VPS KURULUM ÖZETİ"
echo "═══════════════════════════════════════════════════════════════"
echo "🖥️  Node Türü: $NODE_TYPE"
echo "🏷️  Node Adı: $NODE_NAME"
echo "🌐 Network: $NETWORK"
echo "🔌 P2P Port: $P2P_PORT"
echo "🔧 RPC Port: $RPC_PORT"
echo "🔑 RPC Şifresi: $RPC_PASSWORD"
echo "📍 IP Adresi: $(curl -s ifconfig.me)"
echo
echo "🔧 KULLANIŞLI KOMUTLAR:"
echo "  Servis durumu:      sudo systemctl status babachaind"
echo "  Logları izleme:     sudo journalctl -u babachaind -f"
echo "  BabaChain CLI:      sudo -u babachain babachain-cli getblockchaininfo"
echo "  Monitoring:         sudo -u babachain tail -f /home/babachain/monitor.log"
echo
echo "📚 SONRAKİ ADIMLAR:"
echo "  1. Network senkronizasyonunu bekleyin"
echo "  2. Diğer node'ları bu IP'ye bağlayın: $(curl -s ifconfig.me):$P2P_PORT"
echo "  3. Cüzdan oluşturun ve staking'e başlayın"
echo "  4. Monitoring'i takip edin"
echo
echo "═══════════════════════════════════════════════════════════════"

print_success "VPS kurulumu başarıyla tamamlandı! 🎊"