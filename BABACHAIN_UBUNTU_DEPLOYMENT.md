# 🚀 BabaChain Ubuntu Linux Deployment Rehberi

## Genel Bakış
Bu rehber BabaChain blockchain'ini Ubuntu Linux sunucusunda nasıl kuracağınızı ve çalıştıracağınızı gösterir.

## 📋 Sistem Gereksinimleri

### Minimum Gereksinimler
- **İşletim Sistemi**: Ubuntu 20.04 LTS veya üzeri
- **RAM**: 4 GB (8 GB önerilen)
- **Disk**: 100 GB SSD (blockchain büyüdükçe artacak)
- **CPU**: 2 çekirdek (4 çekirdek önerilen)
- **İnternet**: Stabil bağlantı (upload/download hızı önemli)

### Önerilen Sunucu Konfigürasyonu
- **RAM**: 16 GB
- **Disk**: 500 GB NVMe SSD
- **CPU**: 8 çekirdek
- **Bant Genişliği**: 1 Gbps

## 🛠️ Kurulum Adımları

### 1. Sistem Güncellemesi
```bash
# Sistem paketlerini güncelle
sudo apt update && sudo apt upgrade -y

# Gerekli araçları yükle
sudo apt install -y build-essential cmake git wget curl
sudo apt install -y libboost-all-dev libssl-dev libdb++-dev
sudo apt install -y libminiupnpc-dev libzmq3-dev libqt5gui5
sudo apt install -y libqt5core5a libqt5dbus5 qttools5-dev
sudo apt install -y qttools5-dev-tools libprotobuf-dev protobuf-compiler
```

### 2. BabaChain Kullanıcısı Oluşturma
```bash
# BabaChain için özel kullanıcı oluştur
sudo adduser babachain
sudo usermod -aG sudo babachain

# BabaChain kullanıcısına geç
sudo su - babachain
```

### 3. BabaChain Kaynak Kodunu İndirme
```bash
# Ana dizine git
cd ~

# BabaChain repository'sini klonla
git clone https://github.com/BabaChain/BabaChain.git
cd BabaChain

# En son stable branch'e geç
git checkout mainnet-v1.0
```

### 4. BabaChain Derleme
```bash
# Derleme dizinini oluştur
mkdir build && cd build

# CMake ile yapılandır
cmake .. -DCMAKE_BUILD_TYPE=Release

# Derle (CPU çekirdek sayısına göre ayarla)
make -j$(nproc)

# Binary'leri sistem dizinine kopyala
sudo make install
```

### 5. Konfigürasyon Dosyası Oluşturma
```bash
# BabaChain veri dizinini oluştur
mkdir -p ~/.babachain

# Konfigürasyon dosyasını oluştur
cat > ~/.babachain/babachain.conf << 'EOF'
# BabaChain Mainnet Konfigürasyonu
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

# Güvenlik
rpcssl=0
listen=1
discover=1

# Loglama
debug=pos
debug=staking
debug=net

# Mainnet ayarları
testnet=0
regtest=0
EOF

# Konfigürasyon dosyasının izinlerini ayarla
chmod 600 ~/.babachain/babachain.conf
```

### 6. Systemd Servisi Oluşturma
```bash
# Systemd servis dosyasını oluştur
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

# Systemd'yi yeniden yükle
sudo systemctl daemon-reload

# Servisi etkinleştir
sudo systemctl enable babachaind
```

## 🚀 BabaChain'i Başlatma

### 1. İlk Başlatma
```bash
# BabaChain daemon'unu başlat
sudo systemctl start babachaind

# Servis durumunu kontrol et
sudo systemctl status babachaind

# Logları izle
sudo journalctl -u babachaind -f
```

### 2. Blockchain Senkronizasyonu
```bash
# Senkronizasyon durumunu kontrol et
babachain-cli getblockchaininfo

# Ağ bilgilerini kontrol et
babachain-cli getnetworkinfo

# Bağlı peer'ları görüntüle
babachain-cli getpeerinfo
```

### 3. Cüzdan Oluşturma ve Staking
```bash
# Yeni cüzdan oluştur
babachain-cli createwallet "ana_cuzdan"

# Yeni adres oluştur
babachain-cli getnewaddress

# Cüzdanı kilitle (güvenlik için)
babachain-cli encryptwallet "GÜÇLÜ_CÜZDAN_ŞİFRESİ"

# Staking için cüzdanı aç (60 saniye için)
babachain-cli walletpassphrase "GÜÇLÜ_CÜZDAN_ŞİFRESİ" 60

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
```

### Log İzleme
```bash
# Sistem logları
sudo journalctl -u babachaind -f

# BabaChain debug logları
tail -f ~/.babachain/debug.log

# Sadece staking logları
grep "staking" ~/.babachain/debug.log
```

### Servis Yönetimi
```bash
# Servisi durdur
sudo systemctl stop babachaind

# Servisi başlat
sudo systemctl start babachaind

# Servisi yeniden başlat
sudo systemctl restart babachaind

# Servis durumu
sudo systemctl status babachaind
```

## 🛡️ Güvenlik Ayarları

### 1. Firewall Konfigürasyonu
```bash
# UFW firewall'ı etkinleştir
sudo ufw enable

# SSH portunu aç (varsayılan 22)
sudo ufw allow ssh

# BabaChain P2P portunu aç
sudo ufw allow 9999/tcp

# RPC portunu sadece localhost'tan aç (opsiyonel)
sudo ufw allow from 127.0.0.1 to any port 9998

# Firewall durumunu kontrol et
sudo ufw status
```

### 2. Fail2Ban Kurulumu
```bash
# Fail2ban yükle
sudo apt install fail2ban -y

# BabaChain için jail oluştur
sudo tee /etc/fail2ban/jail.d/babachain.conf > /dev/null << 'EOF'
[babachain]
enabled = true
port = 9999
filter = babachain
logpath = /home/babachain/.babachain/debug.log
maxretry = 5
bantime = 3600
EOF

# Fail2ban'ı yeniden başlat
sudo systemctl restart fail2ban
```

### 3. Otomatik Yedekleme
```bash
# Yedekleme scripti oluştur
cat > ~/backup_babachain.sh << 'EOF'
#!/bin/bash
BACKUP_DIR="/home/babachain/backups"
DATE=$(date +%Y%m%d_%H%M%S)

# Yedekleme dizinini oluştur
mkdir -p $BACKUP_DIR

# Cüzdan dosyasını yedekle
cp ~/.babachain/wallet.dat $BACKUP_DIR/wallet_$DATE.dat

# Konfigürasyonu yedekle
cp ~/.babachain/babachain.conf $BACKUP_DIR/babachain_conf_$DATE.conf

# Eski yedekleri temizle (30 günden eski)
find $BACKUP_DIR -name "wallet_*.dat" -mtime +30 -delete
find $BACKUP_DIR -name "babachain_conf_*.conf" -mtime +30 -delete

echo "Yedekleme tamamlandı: $DATE"
EOF

# Script'i çalıştırılabilir yap
chmod +x ~/backup_babachain.sh

# Günlük otomatik yedekleme için crontab ekle
(crontab -l 2>/dev/null; echo "0 2 * * * /home/babachain/backup_babachain.sh") | crontab -
```

## 📊 İzleme ve Monitoring

### 1. Sistem Kaynaklarını İzleme
```bash
# CPU ve RAM kullanımı
htop

# Disk kullanımı
df -h

# BabaChain process'ini izle
ps aux | grep babachaind
```

### 2. Ağ İstatistikleri
```bash
# Ağ bağlantıları
netstat -tulpn | grep 9999

# Bandwidth kullanımı
sudo apt install vnstat -y
vnstat -i eth0
```

### 3. Otomatik Sağlık Kontrolü
```bash
# Sağlık kontrol scripti oluştur
cat > ~/health_check.sh << 'EOF'
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
    if babachain-cli getblockcount > /dev/null 2>&1; then
        echo "$DATE - OK: BabaChain normal çalışıyor" >> $LOG_FILE
    else
        echo "$DATE - UYARI: RPC yanıt vermiyor" >> $LOG_FILE
    fi
fi
EOF

# Script'i çalıştırılabilir yap
chmod +x ~/health_check.sh

# Her 5 dakikada bir kontrol et
(crontab -l 2>/dev/null; echo "*/5 * * * * /home/babachain/health_check.sh") | crontab -
```

## 🔄 Güncelleme Prosedürü

### BabaChain Güncellemesi
```bash
# Mevcut versiyonu kontrol et
babachain-cli getnetworkinfo | grep version

# Daemon'u durdur
sudo systemctl stop babachaind

# Kaynak kodu güncelle
cd ~/BabaChain
git fetch origin
git checkout mainnet-v1.1  # Yeni versiyon

# Yeniden derle
cd build
make -j$(nproc)
sudo make install

# Daemon'u başlat
sudo systemctl start babachaind

# Güncellemeyi doğrula
babachain-cli getnetworkinfo | grep version
```

## 🚨 Sorun Giderme

### Yaygın Sorunlar ve Çözümleri

#### 1. Daemon Başlamıyor
```bash
# Log dosyasını kontrol et
tail -n 50 ~/.babachain/debug.log

# Konfigürasyon dosyasını kontrol et
cat ~/.babachain/babachain.conf

# Port kullanımını kontrol et
sudo netstat -tulpn | grep 9999
```

#### 2. Senkronizasyon Sorunu
```bash
# Peer bağlantılarını kontrol et
babachain-cli getpeerinfo

# Manuel peer ekleme
babachain-cli addnode "seed1.babachain.org:9999" "add"

# Blockchain'i yeniden indirme (son çare)
sudo systemctl stop babachaind
rm -rf ~/.babachain/blocks ~/.babachain/chainstate
sudo systemctl start babachaind
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
# SSD için optimizasyon
echo 'vm.swappiness=10' | sudo tee -a /etc/sysctl.conf
echo 'vm.vfs_cache_pressure=50' | sudo tee -a /etc/sysctl.conf
sudo sysctl -p
```

### 2. Ağ Optimizasyonu
```bash
# TCP ayarlarını optimize et
echo 'net.core.rmem_max = 16777216' | sudo tee -a /etc/sysctl.conf
echo 'net.core.wmem_max = 16777216' | sudo tee -a /etc/sysctl.conf
echo 'net.ipv4.tcp_rmem = 4096 87380 16777216' | sudo tee -a /etc/sysctl.conf
echo 'net.ipv4.tcp_wmem = 4096 65536 16777216' | sudo tee -a /etc/sysctl.conf
sudo sysctl -p
```

## 🎯 Sonuç

Bu rehberi takip ederek BabaChain blockchain'inizi Ubuntu Linux üzerinde başarıyla çalıştırabilirsiniz. Sistem otomatik olarak:

- ✅ Mainnet'e bağlanacak
- ✅ Blockchain'i senkronize edecek  
- ✅ Staking yapmaya başlayacak
- ✅ Ağa katkıda bulunacak

**Önemli Notlar:**
- Cüzdan şifrelerinizi güvenli yerde saklayın
- Düzenli yedekleme yapın
- Sistem güncellemelerini takip edin
- Güvenlik önlemlerini ihmal etmeyin

**Destek için:**
- GitHub: https://github.com/Baba-Chain/BabaChain
- Discord: https://discord.gg/babachain
- Telegram: https://t.me/babachainofficial