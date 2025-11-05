#!/bin/bash

# BabaChain macOS Otomatik Kurulum Scripti
# Bu script BabaChain'i macOS üzerinde otomatik olarak kurar ve yapılandırır

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

# macOS versiyonu kontrolü
macos_version=$(sw_vers -productVersion)
print_status "🍎 BabaChain macOS Kurulum Scripti Başlatılıyor..."
print_status "macOS Versiyonu: $macos_version"

# Minimum macOS versiyonu kontrolü
if [[ $(echo "$macos_version 10.15" | tr " " "\n" | sort -V | head -n1) != "10.15" ]]; then
    print_error "Bu script macOS 10.15 Catalina veya üzeri gerektirir"
    exit 1
fi

# Sistem bilgilerini göster
print_status "Sistem bilgileri:"
echo "Mimari: $(uname -m)"
echo "CPU: $(sysctl -n machdep.cpu.brand_string)"
echo "RAM: $(echo "$(sysctl -n hw.memsize) / 1024 / 1024 / 1024" | bc) GB"
echo "Disk: $(df -h / | awk 'NR==2 {print $4}') boş alan"

# Onay al
read -p "Kuruluma devam etmek istiyor musunuz? (y/N): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    print_warning "Kurulum iptal edildi."
    exit 1
fi

# 1. Xcode Command Line Tools kontrolü ve kurulumu
print_status "🔧 Xcode Command Line Tools kontrol ediliyor..."
if ! xcode-select -p &> /dev/null; then
    print_status "Xcode Command Line Tools yükleniyor..."
    xcode-select --install
    print_warning "Xcode Command Line Tools kurulumu tamamlandıktan sonra scripti tekrar çalıştırın"
    exit 1
else
    print_success "Xcode Command Line Tools zaten yüklü"
fi

# 2. Homebrew kontrolü ve kurulumu
print_status "🍺 Homebrew kontrol ediliyor..."
if ! command -v brew &> /dev/null; then
    print_status "Homebrew yükleniyor..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    
    # PATH'e ekle
    if [[ $(uname -m) == "arm64" ]]; then
        echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zprofile
        eval "$(/opt/homebrew/bin/brew shellenv)"
    else
        echo 'eval "$(/usr/local/bin/brew shellenv)"' >> ~/.zprofile
        eval "$(/usr/local/bin/brew shellenv)"
    fi
    
    print_success "Homebrew yüklendi"
else
    print_success "Homebrew zaten yüklü"
    print_status "Homebrew güncelleniyor..."
    brew update
fi

# 3. Gerekli paketleri yükle
print_status "📦 Gerekli paketler yükleniyor..."
brew install cmake boost openssl berkeley-db miniupnpc zeromq qt5 protobuf pkg-config libevent qrencode python3 git wget curl

print_success "Gerekli paketler yüklendi"

# 4. BabaChain kaynak kodunu indir
print_status "📥 BabaChain kaynak kodu indiriliyor..."
cd ~
if [ ! -d "BabaChain" ]; then
    git clone https://github.com/BabaChain/BabaChain.git
    print_success "Kaynak kod indirildi"
else
    print_warning "Kaynak kod zaten mevcut, güncelleniyor..."
    cd BabaChain
    git pull origin main
    cd ~
fi

# 5. BabaChain'i derle
print_status "🔨 BabaChain derleniyor... (Bu işlem biraz zaman alabilir)"
cd ~/BabaChain

# Build dizinini oluştur
mkdir -p build
cd build

# CMake ile yapılandır
if [[ $(uname -m) == "arm64" ]]; then
    # Apple Silicon
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DOPENSSL_ROOT_DIR=/opt/homebrew/opt/openssl \
             -DBOOST_ROOT=/opt/homebrew/opt/boost
else
    # Intel Mac
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl \
             -DBOOST_ROOT=/usr/local/opt/boost
fi

# Derle
CPU_CORES=$(sysctl -n hw.ncpu)
print_status "🔧 $CPU_CORES çekirdek kullanılarak derleniyor..."
make -j$CPU_CORES

# Binary'leri yükle
sudo make install

print_success "BabaChain başarıyla derlendi ve yüklendi"

# 6. Konfigürasyon dizinini oluştur
print_status "⚙️ Konfigürasyon dosyaları oluşturuluyor..."
mkdir -p ~/Library/Application\ Support/BabaChain

# RPC şifresi oluştur
RPC_PASSWORD=$(openssl rand -base64 32)

# Konfigürasyon dosyasını oluştur
cat > ~/Library/Application\ Support/BabaChain/babachain.conf << EOF
# BabaChain macOS Konfigürasyonu
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

# macOS optimizasyonları
uacomment=BabaChain-macOS-Node
EOF

# Konfigürasyon dosyasının izinlerini ayarla
chmod 600 ~/Library/Application\ Support/BabaChain/babachain.conf

print_success "Konfigürasyon dosyası oluşturuldu"
print_warning "RPC şifresi: $RPC_PASSWORD (Bu şifreyi güvenli bir yerde saklayın!)"

# 7. LaunchAgent servisi oluştur
print_status "🔧 LaunchAgent servisi oluşturuluyor..."
mkdir -p ~/Library/LaunchAgents

cat > ~/Library/LaunchAgents/org.babachain.babachaind.plist << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>org.babachain.babachaind</string>
    <key>ProgramArguments</key>
    <array>
        <string>/usr/local/bin/babachaind</string>
        <string>-daemon</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <true/>
    <key>StandardOutPath</key>
    <string>$HOME/Library/Logs/BabaChain/babachaind.log</string>
    <key>StandardErrorPath</key>
    <string>$HOME/Library/Logs/BabaChain/babachaind_error.log</string>
    <key>WorkingDirectory</key>
    <string>$HOME</string>
</dict>
</plist>
EOF

# Log dizinini oluştur
mkdir -p ~/Library/Logs/BabaChain

# LaunchAgent'ı yükle
launchctl load ~/Library/LaunchAgents/org.babachain.babachaind.plist

print_success "LaunchAgent servisi oluşturuldu ve yüklendi"

# 8. Firewall ayarları
print_status "🛡️ Firewall ayarları yapılandırılıyor..."
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --setglobalstate on
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --add /usr/local/bin/babachaind
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --unblockapp /usr/local/bin/babachaind

print_success "Firewall kuralları eklendi"

# 9. Yedekleme scripti oluştur
print_status "💾 Yedekleme scripti oluşturuluyor..."
cat > ~/backup_babachain.sh << 'EOF'
#!/bin/bash
BACKUP_DIR="$HOME/BabaChain_Backups"
DATE=$(date +%Y%m%d_%H%M%S)

mkdir -p "$BACKUP_DIR"

# Cüzdan dosyasını yedekle (eğer varsa)
if [ -f "$HOME/Library/Application Support/BabaChain/wallet.dat" ]; then
    cp "$HOME/Library/Application Support/BabaChain/wallet.dat" "$BACKUP_DIR/wallet_$DATE.dat"
fi

# Konfigürasyonu yedekle
cp "$HOME/Library/Application Support/BabaChain/babachain.conf" "$BACKUP_DIR/babachain_conf_$DATE.conf"

# Eski yedekleri temizle (30 günden eski)
find "$BACKUP_DIR" -name "wallet_*.dat" -mtime +30 -delete 2>/dev/null || true
find "$BACKUP_DIR" -name "babachain_conf_*.conf" -mtime +30 -delete 2>/dev/null || true

echo "$(date): Yedekleme tamamlandı"
EOF

chmod +x ~/backup_babachain.sh

# Günlük yedekleme cron job'u ekle
(crontab -l 2>/dev/null; echo "0 2 * * * $HOME/backup_babachain.sh >> $HOME/backup.log 2>&1") | crontab -

print_success "Yedekleme scripti oluşturuldu ve zamanlandı"

# 10. Sağlık kontrol scripti oluştur
print_status "🏥 Sağlık kontrol scripti oluşturuluyor..."
cat > ~/health_check_babachain.sh << 'EOF'
#!/bin/bash
LOG_FILE="$HOME/Library/Logs/BabaChain/health_check.log"
DATE=$(date '+%Y-%m-%d %H:%M:%S')

# BabaChain daemon'unun çalışıp çalışmadığını kontrol et
if ! pgrep -x "babachaind" > /dev/null; then
    echo "$DATE - HATA: BabaChain daemon çalışmıyor!" >> "$LOG_FILE"
    launchctl start org.babachain.babachaind
    echo "$DATE - BabaChain daemon yeniden başlatıldı" >> "$LOG_FILE"
else
    # RPC bağlantısını test et
    if /usr/local/bin/babachain-cli getblockcount > /dev/null 2>&1; then
        BLOCK_COUNT=$(/usr/local/bin/babachain-cli getblockcount)
        echo "$DATE - OK: BabaChain çalışıyor, blok: $BLOCK_COUNT" >> "$LOG_FILE"
    else
        echo "$DATE - UYARI: RPC yanıt vermiyor" >> "$LOG_FILE"
    fi
fi
EOF

chmod +x ~/health_check_babachain.sh

# Her 5 dakikada bir sağlık kontrolü
(crontab -l 2>/dev/null; echo "*/5 * * * * $HOME/health_check_babachain.sh") | crontab -

print_success "Sağlık kontrol scripti oluşturuldu"

# 11. Kullanışlı aliaslar oluştur
print_status "🔧 Kullanışlı komut aliasları oluşturuluyor..."
cat >> ~/.zshrc << 'EOF'

# BabaChain aliasları
alias baba-cli='/usr/local/bin/babachain-cli'
alias baba-qt='/usr/local/bin/babachain-qt'
alias baba-status='baba-cli getblockchaininfo'
alias baba-balance='baba-cli getbalance'
alias baba-staking='baba-cli getstakinginfo'
alias baba-peers='baba-cli getpeerinfo | grep addr'
alias baba-logs='tail -f ~/Library/Application\ Support/BabaChain/debug.log'
alias baba-start='launchctl start org.babachain.babachaind'
alias baba-stop='launchctl stop org.babachain.babachaind'
alias baba-restart='launchctl stop org.babachain.babachaind && launchctl start org.babachain.babachaind'
alias baba-service='launchctl list | grep babachain'
EOF

print_success "Komut aliasları eklendi"

# 12. BabaChain'i başlat
print_status "🚀 BabaChain daemon'u başlatılıyor..."
launchctl start org.babachain.babachaind

# Başlatma kontrolü
sleep 5
if launchctl list | grep -q "org.babachain.babachaind"; then
    print_success "BabaChain daemon başarıyla başlatıldı!"
else
    print_error "BabaChain daemon başlatılamadı. Logları kontrol edin:"
    echo "tail -f ~/Library/Logs/BabaChain/babachaind.log"
    exit 1
fi

# 13. Kurulum özeti
print_success "🎉 BabaChain kurulumu tamamlandı!"
echo
echo "═══════════════════════════════════════════════════════════════"
echo "                    KURULUM ÖZETİ"
echo "═══════════════════════════════════════════════════════════════"
echo
echo "📍 Kurulum Dizini: ~/BabaChain"
echo "📁 Veri Dizini: ~/Library/Application Support/BabaChain"
echo "⚙️ Konfigürasyon: ~/Library/Application Support/BabaChain/babachain.conf"
echo "🔑 RPC Şifresi: $RPC_PASSWORD"
echo
echo "🔧 KULLANIŞLI KOMUTLAR:"
echo "  LaunchAgent durumu: launchctl list | grep babachain"
echo "  Logları izleme:     tail -f ~/Library/Logs/BabaChain/babachaind.log"
echo "  Blockchain bilgisi: babachain-cli getblockchaininfo"
echo "  Staking durumu:     babachain-cli getstakinginfo"
echo "  Bakiye kontrolü:    babachain-cli getbalance"
echo "  GUI Wallet:         babachain-qt"
echo
echo "📚 SONRAKI ADIMLAR:"
echo "  1. Blockchain senkronizasyonunu bekleyin (birkaç saat sürebilir)"
echo "  2. Cüzdan oluşturun: babachain-cli createwallet \"ana_cuzdan\""
echo "  3. Adres oluşturun: babachain-cli getnewaddress"
echo "  4. BabaChain token'larınızı bu adrese gönderin"
echo "  5. Staking'e başlayın!"
echo
echo "🔗 DESTEK:"
echo "  GitHub: https://github.com/BabaChain/BabaChain"
echo "  Discord: https://discord.gg/babachain"
echo "  Telegram: https://t.me/babachainofficial"
echo
echo "═══════════════════════════════════════════════════════════════"

# Terminal'i yeniden başlatma önerisi
echo
print_warning "Yeni aliasları kullanmak için terminal'i yeniden başlatın veya 'source ~/.zshrc' komutunu çalıştırın"

print_success "Kurulum başarıyla tamamlandı! 🎉"