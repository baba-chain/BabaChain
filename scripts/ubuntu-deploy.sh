#!/bin/bash

# BabaChain Ubuntu Otomatik Kurulum Scripti
# Bu script BabaChain'i Ubuntu Linux üzerinde otomatik olarak kurar ve yapılandırır

set -e

# Renkler
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
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

# Root kontrolü
if [[ $EUID -eq 0 ]]; then
   print_error "Bu scripti root kullanıcısı olarak çalıştırmayın!"
   exit 1
fi

print_status "🚀 BabaChain Ubuntu Kurulum Scripti Başlatılıyor..."

# Sistem bilgilerini kontrol et
print_status "Sistem bilgileri kontrol ediliyor..."
echo "İşletim Sistemi: $(lsb_release -d | cut -f2)"
echo "Çekirdek: $(uname -r)"
echo "Mimari: $(uname -m)"
echo "RAM: $(free -h | awk '/^Mem:/ {print $2}')"
echo "Disk: $(df -h / | awk 'NR==2 {print $4}') boş alan"

# Onay al
read -p "Kuruluma devam etmek istiyor musunuz? (y/N): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    print_warning "Kurulum iptal edildi."
    exit 1
fi

# 1. Sistem güncellemesi
print_status "📦 Sistem paketleri güncelleniyor..."
sudo apt update && sudo apt upgrade -y

# 2. Gerekli paketleri yükle
print_status "🛠️ Gerekli paketler yükleniyor..."
sudo apt install -y \
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
    libqrencode-dev

print_success "Gerekli paketler yüklendi"

# 3. BabaChain kullanıcısı oluştur (eğer yoksa)
if ! id "babachain" &>/dev/null; then
    print_status "👤 BabaChain kullanıcısı oluşturuluyor..."
    sudo adduser --disabled-password --gecos "" babachain
    sudo usermod -aG sudo babachain
    print_success "BabaChain kullanıcısı oluşturuldu"
else
    print_warning "BabaChain kullanıcısı zaten mevcut"
fi

# 4. BabaChain kaynak kodunu indir
print_status "📥 BabaChain kaynak kodu indiriliyor..."
if [ ! -d "/home/babachain/BabaChain" ]; then
    sudo -u babachain git clone https://github.com/BabaChain/BabaChain.git /home/babachain/BabaChain
    print_success "Kaynak kod indirildi"
else
    print_warning "Kaynak kod zaten mevcut, güncelleniyor..."
    sudo -u babachain git -C /home/babachain/BabaChain pull origin main
fi

# 5. BabaChain'i derle
print_status "🔨 BabaChain derleniyor... (Bu işlem biraz zaman alabilir)"
cd /home/babachain/BabaChain

# Build dizinini oluştur
sudo -u babachain mkdir -p build
cd build

# CMake ile yapılandır
sudo -u babachain cmake .. -DCMAKE_BUILD_TYPE=Release

# Derle
CPU_CORES=$(nproc)
print_status "🔧 $CPU_CORES çekirdek kullanılarak derleniyor..."
sudo -u babachain make -j$CPU_CORES

# Binary'leri yükle
sudo make install

print_success "BabaChain başarıyla derlendi ve yüklendi"

# 6. Konfigürasyon dizinini oluştur
print_status "⚙️ Konfigürasyon dosyaları oluşturuluyor..."
sudo -u babachain mkdir -p /home/babachain/.babachain

# RPC şifresi oluştur
RPC_PASSWORD=$(openssl rand -base64 32)

# Konfigürasyon dosyasını oluştur
sudo -u babachain tee /home/babachain/.babachain/babachain.conf > /dev/null << EOF
# BabaChain Mainnet Konfigürasyonu
# Otomatik olarak oluşturuldu: $(date)

# RPC ayarları
rpcuser=babachain_user
rpcpassword=$RPC_PASSWORD
rpcport=9998
rpcbind=127.0.0.1
rpcallowip=127.0.0.1

# Ağ ayarları
port=9999
listen=1
discover=1
upnp=1

# PoS staking ayarları
staking=1
stakegen=1
reservebalance=0

# Seed node'lar
addnode=seed1.babachain.org:9999
addnode=seed2.babachain.org:9999
addnode=seed3.babachain.org:9999
addnode=node1.babachain.network:9999
addnode=node2.babachain.network:9999

# Performans ayarları
maxconnections=125
timeout=5000
dbcache=300
maxmempool=300

# Güvenlik
rpcssl=0

# Loglama
debug=pos
debug=staking
debug=net

# Mainnet
testnet=0
regtest=0

# Türkçe destek
uacomment=BabaChain-Turkish-Node
EOF

# Konfigürasyon dosyasının izinlerini ayarla
sudo chmod 600 /home/babachain/.babachain/babachain.conf
sudo chown babachain:babachain /home/babachain/.babachain/babachain.conf

print_success "Konfigürasyon dosyası oluşturuldu"
print_warning "RPC şifresi: $RPC_PASSWORD (Bu şifreyi güvenli bir yerde saklayın!)"

# 7. Systemd servisi oluştur
print_status "🔧 Systemd servisi oluşturuluyor..."
sudo tee /etc/systemd/system/babachaind.service > /dev/null << 'EOF'
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

# Güvenlik ayarları
NoNewPrivileges=true
PrivateDevices=true
ProtectHome=true
ProtectSystem=strict
ReadWritePaths=/home/babachain/.babachain

[Install]
WantedBy=multi-user.target
EOF

# Systemd'yi yeniden yükle ve servisi etkinleştir
sudo systemctl daemon-reload
sudo systemctl enable babachaind

print_success "Systemd servisi oluşturuldu ve etkinleştirildi"

# 8. Firewall ayarları
print_status "🛡️ Firewall ayarları yapılandırılıyor..."
if command -v ufw &> /dev/null; then
    sudo ufw allow 9999/tcp comment "BabaChain P2P"
    print_success "Firewall kuralları eklendi"
else
    print_warning "UFW bulunamadı, firewall kurallarını manuel olarak ayarlayın"
fi

# 9. Yedekleme scripti oluştur
print_status "💾 Yedekleme scripti oluşturuluyor..."
sudo -u babachain tee /home/babachain/backup_babachain.sh > /dev/null << 'EOF'
#!/bin/bash
BACKUP_DIR="/home/babachain/backups"
DATE=$(date +%Y%m%d_%H%M%S)

mkdir -p $BACKUP_DIR

# Cüzdan dosyasını yedekle (eğer varsa)
if [ -f ~/.babachain/wallet.dat ]; then
    cp ~/.babachain/wallet.dat $BACKUP_DIR/wallet_$DATE.dat
fi

# Konfigürasyonu yedekle
cp ~/.babachain/babachain.conf $BACKUP_DIR/babachain_conf_$DATE.conf

# Eski yedekleri temizle (30 günden eski)
find $BACKUP_DIR -name "wallet_*.dat" -mtime +30 -delete 2>/dev/null || true
find $BACKUP_DIR -name "babachain_conf_*.conf" -mtime +30 -delete 2>/dev/null || true

echo "$(date): Yedekleme tamamlandı"
EOF

sudo chmod +x /home/babachain/backup_babachain.sh

# Günlük yedekleme cron job'u ekle
(sudo -u babachain crontab -l 2>/dev/null; echo "0 2 * * * /home/babachain/backup_babachain.sh >> /home/babachain/backup.log 2>&1") | sudo -u babachain crontab -

print_success "Yedekleme scripti oluşturuldu ve zamanlandı"

# 10. Sağlık kontrol scripti oluştur
print_status "🏥 Sağlık kontrol scripti oluşturuluyor..."
sudo -u babachain tee /home/babachain/health_check.sh > /dev/null << 'EOF'
#!/bin/bash
LOG_FILE="/home/babachain/health_check.log"
DATE=$(date '+%Y-%m-%d %H:%M:%S')

# BabaChain daemon'unun çalışıp çalışmadığını kontrol et
if ! pgrep -x "babachaind" > /dev/null; then
    echo "$DATE - HATA: BabaChain daemon çalışmıyor!" >> $LOG_FILE
    sudo systemctl start babachaind
    echo "$DATE - BabaChain daemon yeniden başlatıldı" >> $LOG_FILE
else
    # RPC bağlantısını test et
    if /usr/local/bin/babachain-cli getblockcount > /dev/null 2>&1; then
        BLOCK_COUNT=$(/usr/local/bin/babachain-cli getblockcount)
        echo "$DATE - OK: BabaChain çalışıyor, blok: $BLOCK_COUNT" >> $LOG_FILE
    else
        echo "$DATE - UYARI: RPC yanıt vermiyor" >> $LOG_FILE
    fi
fi
EOF

sudo chmod +x /home/babachain/health_check.sh

# Her 5 dakikada bir sağlık kontrolü
(sudo -u babachain crontab -l 2>/dev/null; echo "*/5 * * * * /home/babachain/health_check.sh") | sudo -u babachain crontab -

print_success "Sağlık kontrol scripti oluşturuldu"

# 11. Kullanışlı aliaslar oluştur
print_status "🔧 Kullanışlı komut aliasları oluşturuluyor..."
sudo -u babachain tee -a /home/babachain/.bashrc > /dev/null << 'EOF'

# BabaChain aliasları
alias baba-cli='/usr/local/bin/babachain-cli'
alias baba-status='baba-cli getblockchaininfo'
alias baba-balance='baba-cli getbalance'
alias baba-staking='baba-cli getstakinginfo'
alias baba-peers='baba-cli getpeerinfo | grep addr'
alias baba-logs='tail -f ~/.babachain/debug.log'
alias baba-start='sudo systemctl start babachaind'
alias baba-stop='sudo systemctl stop babachaind'
alias baba-restart='sudo systemctl restart babachaind'
alias baba-service='sudo systemctl status babachaind'
EOF

print_success "Komut aliasları eklendi"

# 12. BabaChain'i başlat
print_status "🚀 BabaChain daemon'u başlatılıyor..."
sudo systemctl start babachaind

# Başlatma kontrolü
sleep 5
if sudo systemctl is-active --quiet babachaind; then
    print_success "BabaChain daemon başarıyla başlatıldı!"
else
    print_error "BabaChain daemon başlatılamadı. Logları kontrol edin:"
    echo "sudo journalctl -u babachaind -f"
    exit 1
fi

# 13. Kurulum özeti
print_success "🎉 BabaChain kurulumu tamamlandı!"
echo
echo "═══════════════════════════════════════════════════════════════"
echo "                    KURULUM ÖZETİ"
echo "═══════════════════════════════════════════════════════════════"
echo
echo "📍 Kurulum Dizini: /home/babachain/BabaChain"
echo "📁 Veri Dizini: /home/babachain/.babachain"
echo "⚙️ Konfigürasyon: /home/babachain/.babachain/babachain.conf"
echo "🔑 RPC Şifresi: $RPC_PASSWORD"
echo
echo "🔧 KULLANIŞLI KOMUTLAR:"
echo "  Durum kontrolü:     sudo systemctl status babachaind"
echo "  Logları izleme:     sudo journalctl -u babachaind -f"
echo "  Blockchain bilgisi: sudo -u babachain babachain-cli getblockchaininfo"
echo "  Staking durumu:     sudo -u babachain babachain-cli getstakinginfo"
echo "  Bakiye kontrolü:    sudo -u babachain babachain-cli getbalance"
echo
echo "📚 SONRAKI ADIMLAR:"
echo "  1. Blockchain senkronizasyonunu bekleyin (birkaç saat sürebilir)"
echo "  2. Cüzdan oluşturun: sudo -u babachain babachain-cli createwallet \"ana_cuzdan\""
echo "  3. Adres oluşturun: sudo -u babachain babachain-cli getnewaddress"
echo "  4. BabaChain token'larınızı bu adrese gönderin"
echo "  5. Staking'e başlayın!"
echo
echo "🔗 DESTEK:"
echo "  GitHub: https://github.com/BabaChain/BabaChain"
echo "  Discord: https://discord.gg/babachain"
echo "  Telegram: https://t.me/babachainofficial"
echo
echo "═══════════════════════════════════════════════════════════════"

# BabaChain kullanıcısına geçiş önerisi
echo
print_warning "BabaChain komutlarını çalıştırmak için 'sudo su - babachain' komutu ile BabaChain kullanıcısına geçin"
print_warning "Veya komutları 'sudo -u babachain' ile başlatın"

print_success "Kurulum başarıyla tamamlandı! 🎉"