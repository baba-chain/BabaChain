# 🍎 BabaChain macOS Deployment Rehberi

## Genel Bakış
Bu rehber BabaChain blockchain'ini macOS üzerinde nasıl kuracağınızı ve çalıştıracağınızı gösterir.

## 📋 Sistem Gereksinimleri

### Minimum Gereksinimler
- **macOS**: 10.15 Catalina veya üzeri
- **RAM**: 8 GB (16 GB önerilen)
- **Disk**: 100 GB boş alan (SSD önerilen)
- **CPU**: Intel/Apple Silicon (M1/M2/M3)
- **Xcode Command Line Tools**: Gerekli

### Önerilen Konfigürasyon
- **macOS**: 13.0 Ventura veya üzeri
- **RAM**: 32 GB
- **Disk**: 500 GB NVMe SSD
- **CPU**: Apple M2 Pro veya üzeri

## 🛠️ Kurulum Adımları

### 1. Xcode Command Line Tools Kurulumu
```bash
# Xcode Command Line Tools'u yükle
xcode-select --install

# Kurulumu doğrula
xcode-select -p
```

### 2. Homebrew Kurulumu
```bash
# Homebrew'i yükle (eğer yoksa)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# PATH'e ekle (Apple Silicon için)
echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zprofile
eval "$(/opt/homebrew/bin/brew shellenv)"

# Intel Mac için
echo 'eval "$(/usr/local/bin/brew shellenv)"' >> ~/.zprofile
eval "$(/usr/local/bin/brew shellenv)"
```

### 3. Gerekli Paketleri Yükleme
```bash
# Gerekli paketleri yükle
brew install cmake boost openssl berkeley-db miniupnpc zeromq qt6 protobuf pkg-config libevent qrencode

# Python ve diğer araçlar
brew install python3 git wget curl
```

### 4. BabaChain Kaynak Kodunu İndirme
```bash
# Ana dizine git
cd ~

# BabaChain repository'sini klonla
git clone https://github.com/Baba-Chain/BabaChain.git
cd BabaChain

# En son stable branch'e geç
git checkout mainnet-v1.0
```

### 5. BabaChain Derleme
```bash
# Derleme dizinini oluştur
mkdir build && cd build

# CMake ile yapılandır (Intel Mac)
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl \
         -DBOOST_ROOT=/usr/local/opt/boost

# CMake ile yapılandır (Apple Silicon)
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DOPENSSL_ROOT_DIR=/opt/homebrew/opt/openssl \
         -DBOOST_ROOT=/opt/homebrew/opt/boost

# Derle
make -j$(sysctl -n hw.ncpu)

# Binary'leri yükle
sudo make install
```

### 6. Konfigürasyon Dosyası Oluşturma
```bash
# BabaChain veri dizinini oluştur
mkdir -p ~/Library/Application\ Support/BabaChain

# Konfigürasyon dosyasını oluştur
cat > ~/Library/Application\ Support/BabaChain/babachain.conf << 'EOF'
# BabaChain macOS Konfigürasyonu
# Ağ ayarları
rpcuser=babachain_user
rpcpassword=GÜÇLÜ_ŞİFRE_BURAYA
rpcport=9998
port=9999
rpcbind=127.0.0.1
rpcallowip=127.0.0.1

# PoS staking ayarları
staking=1
stakegen=1
reservebalance=0

# Ağ keşfi - Seed node'lar
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

# macOS optimizasyonları
listen=1
discover=1
upnp=1

# Loglama
debug=pos
debug=staking
debug=net

# Mainnet ayarları
testnet=0
regtest=0
EOF

# Konfigürasyon dosyasının izinlerini ayarla
chmod 600 ~/Library/Application\ Support/BabaChain/babachain.conf
```

### 7. LaunchAgent Servisi Oluşturma (Otomatik Başlatma)
```bash
# LaunchAgent dizinini oluştur
mkdir -p ~/Library/LaunchAgents

# LaunchAgent plist dosyasını oluştur
cat > ~/Library/LaunchAgents/org.babachain.babachaind.plist << 'EOF'
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
    <string>/Users/$(whoami)/Library/Logs/BabaChain/babachaind.log</string>
    <key>StandardErrorPath</key>
    <string>/Users/$(whoami)/Library/Logs/BabaChain/babachaind_error.log</string>
    <key>WorkingDirectory</key>
    <string>/Users/$(whoami)</string>
</dict>
</plist>
EOF

# Log dizinini oluştur
mkdir -p ~/Library/Logs/BabaChain

# LaunchAgent'ı yükle
launchctl load ~/Library/LaunchAgents/org.babachain.babachaind.plist
```

## 🚀 BabaChain'i Başlatma

### 1. Manuel Başlatma
```bash
# BabaChain daemon'unu başlat
babachaind -daemon

# Durumu kontrol et
babachain-cli getblockchaininfo
```

### 2. LaunchAgent ile Otomatik Başlatma
```bash
# LaunchAgent'ı başlat
launchctl start org.babachain.babachaind

# Durumu kontrol et
launchctl list | grep babachain

# Logları görüntüle
tail -f ~/Library/Logs/BabaChain/babachaind.log
```

### 3. Cüzdan Oluşturma ve Staking
```bash
# Yeni cüzdan oluştur
babachain-cli createwallet "ana_cuzdan"

# Yeni adres oluştur
babachain-cli getnewaddress

# Cüzdanı şifrele
babachain-cli encryptwallet "GÜÇLÜ_CÜZDAN_ŞİFRESİ"

# Staking için cüzdanı aç
babachain-cli walletpassphrase "GÜÇLÜ_CÜZDAN_ŞİFRESİ" 999999999 true

# Staking durumunu kontrol et
babachain-cli getstakinginfo
```

## 🔧 Günlük İşlemler

### Sistem Durumu Kontrolü
```bash
# Node durumu
babachain-cli getinfo

# Blockchain durumu
babachain-cli getblockchaininfo

# Staking durumu
babachain-cli getstakinginfo

# Cüzdan bakiyesi
babachain-cli getbalance

# Peer bağlantıları
babachain-cli getpeerinfo
```

### Log İzleme
```bash
# BabaChain debug logları
tail -f ~/Library/Application\ Support/BabaChain/debug.log

# LaunchAgent logları
tail -f ~/Library/Logs/BabaChain/babachaind.log

# Sadece staking logları
grep "staking" ~/Library/Application\ Support/BabaChain/debug.log
```

### Servis Yönetimi
```bash
# Servisi durdur
launchctl stop org.babachain.babachaind

# Servisi başlat
launchctl start org.babachain.babachaind

# Servisi yeniden yükle
launchctl unload ~/Library/LaunchAgents/org.babachain.babachaind.plist
launchctl load ~/Library/LaunchAgents/org.babachain.babachaind.plist
```

## 🛡️ Güvenlik Ayarları

### 1. Firewall Konfigürasyonu
```bash
# macOS Firewall'ı etkinleştir
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --setglobalstate on

# BabaChain'e izin ver
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --add /usr/local/bin/babachaind
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --unblockapp /usr/local/bin/babachaind
```

### 2. Otomatik Yedekleme
```bash
# Yedekleme scripti oluştur
cat > ~/backup_babachain.sh << 'EOF'
#!/bin/bash
BACKUP_DIR="$HOME/BabaChain_Backups"
DATE=$(date +%Y%m%d_%H%M%S)

# Yedekleme dizinini oluştur
mkdir -p "$BACKUP_DIR"

# Cüzdan dosyasını yedekle
if [ -f "$HOME/Library/Application Support/BabaChain/wallet.dat" ]; then
    cp "$HOME/Library/Application Support/BabaChain/wallet.dat" "$BACKUP_DIR/wallet_$DATE.dat"
fi

# Konfigürasyonu yedekle
cp "$HOME/Library/Application Support/BabaChain/babachain.conf" "$BACKUP_DIR/babachain_conf_$DATE.conf"

# Eski yedekleri temizle (30 günden eski)
find "$BACKUP_DIR" -name "wallet_*.dat" -mtime +30 -delete
find "$BACKUP_DIR" -name "babachain_conf_*.conf" -mtime +30 -delete

echo "Yedekleme tamamlandı: $DATE"
EOF

# Script'i çalıştırılabilir yap
chmod +x ~/backup_babachain.sh

# Günlük otomatik yedekleme için crontab ekle
(crontab -l 2>/dev/null; echo "0 2 * * * $HOME/backup_babachain.sh") | crontab -
```

### 3. Sağlık Kontrolü
```bash
# Sağlık kontrol scripti oluştur
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

# Script'i çalıştırılabilir yap
chmod +x ~/health_check_babachain.sh

# Her 5 dakikada bir kontrol et
(crontab -l 2>/dev/null; echo "*/5 * * * * $HOME/health_check_babachain.sh") | crontab -
```

## 📊 İzleme ve Monitoring

### 1. Sistem Kaynaklarını İzleme
```bash
# CPU ve RAM kullanımı
top -pid $(pgrep babachaind)

# Disk kullanımı
df -h

# BabaChain process'ini izle
ps aux | grep babachaind
```

### 2. Ağ İstatistikleri
```bash
# Ağ bağlantıları
netstat -an | grep 9999

# Port kullanımı
lsof -i :9999
```

## 🔄 Güncelleme Prosedürü

### BabaChain Güncellemesi
```bash
# Mevcut versiyonu kontrol et
babachain-cli getnetworkinfo | grep version

# Daemon'u durdur
launchctl stop org.babachain.babachaind

# Kaynak kodu güncelle
cd ~/BabaChain
git fetch origin
git checkout mainnet-v1.1  # Yeni versiyon

# Yeniden derle
cd build
make -j$(sysctl -n hw.ncpu)
sudo make install

# Daemon'u başlat
launchctl start org.babachain.babachaind

# Güncellemeyi doğrula
babachain-cli getnetworkinfo | grep version
```

## 🚨 Sorun Giderme

### Yaygın Sorunlar ve Çözümleri

#### 1. Derleme Hatası
```bash
# Xcode Command Line Tools'u güncelle
sudo xcode-select --install

# Homebrew paketlerini güncelle
brew update && brew upgrade

# OpenSSL path sorunları için
export LDFLAGS="-L/opt/homebrew/opt/openssl/lib"
export CPPFLAGS="-I/opt/homebrew/opt/openssl/include"
```

#### 2. Daemon Başlamıyor
```bash
# Log dosyasını kontrol et
tail -n 50 ~/Library/Application\ Support/BabaChain/debug.log

# Port kullanımını kontrol et
lsof -i :9999

# Konfigürasyon dosyasını kontrol et
cat ~/Library/Application\ Support/BabaChain/babachain.conf
```

#### 3. Staking Çalışmıyor
```bash
# Staking durumunu detaylı kontrol et
babachain-cli getstakinginfo

# Cüzdan kilidini kontrol et
babachain-cli walletinfo

# Cüzdanı staking için aç
babachain-cli walletpassphrase "ŞİFRE" 999999999 true
```

## 📈 Performans Optimizasyonu

### 1. SSD Optimizasyonu
```bash
# TRIM'i etkinleştir (eğer SSD kullanıyorsanız)
sudo trimforce enable
```

### 2. Ağ Optimizasyonu
```bash
# TCP ayarlarını optimize et
sudo sysctl -w net.inet.tcp.sendspace=65536
sudo sysctl -w net.inet.tcp.recvspace=65536
```

## 🎯 Sonuç

Bu rehberi takip ederek BabaChain blockchain'inizi macOS üzerinde başarıyla çalıştırabilirsiniz. Sistem otomatik olarak:

- ✅ Mainnet'e bağlanacak
- ✅ Blockchain'i senkronize edecek  
- ✅ Staking yapmaya başlayacak
- ✅ Ağa katkıda bulunacak

**Önemli Notlar:**
- Cüzdan şifrelerinizi güvenli yerde saklayın
- Düzenli yedekleme yapın
- Sistem güncellemelerini takip edin
- macOS güvenlik ayarlarını ihmal etmeyin

**Destek için:**
- GitHub: https://github.com/Baba-Chain/BabaChain
- Discord: https://discord.gg/babachain
- Telegram: https://t.me/babachainofficial
