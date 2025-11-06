# 🚀 BabaChain VPS Hızlı Kurulum Rehberi

## ❌ Sorun: GitHub URL Hatası

Aldığınız hata GitHub'dan raw dosya indirme URL'inin yanlış olmasından kaynaklanıyor.

### Yanlış URL (HTML döndürür):
```bash
# Bu çalışmaz - HTML sayfası döndürür
curl -fsSL https://github.com/baba-chain/BabaChain/blob/development/scripts/vps-quick-setup.sh
```

### ✅ Doğru Çözümler:

## Çözüm 1: Manuel İndirme ve Çalıştırma (ÖNERİLEN)

```bash
# 1. Repository'yi klonla
git clone https://github.com/Baba-Chain/BabaChain.git
cd BabaChain

# 2. Script'i çalıştır
chmod +x scripts/vps-quick-setup.sh
sudo ./scripts/vps-quick-setup.sh seed mainnet-seed1

# Veya parametrelerle:
sudo ./scripts/vps-quick-setup.sh seed node1 mainnet
```

## Çözüm 2: Raw GitHub URL (Eğer repository public ise)

```bash
# Doğru raw URL formatı (eğer repository public ise):
curl -fsSL https://raw.githubusercontent.com/Baba-Chain/BabaChain/main/scripts/vps-quick-setup.sh | sudo bash -s seed mainnet-seed1
```

## Çözüm 3: Adım Adım Manuel Kurulum

### Ubuntu VPS'te Adım Adım:

```bash
# 1. Sistem güncelle
apt update && apt upgrade -y

# 2. Gerekli paketleri yükle
apt install -y build-essential cmake git wget curl \
    libboost-all-dev libssl-dev libdb++-dev \
    libminiupnpc-dev libzmq3-dev libqt5gui5 \
    libqt5core5a libqt5dbus5 qttools5-dev \
    qttools5-dev-tools libprotobuf-dev \
    protobuf-compiler pkg-config libevent-dev \
    libqrencode-dev ufw fail2ban

# 3. BabaChain kullanıcısı oluştur
adduser --disabled-password --gecos "" babachain
usermod -aG sudo babachain

# 4. BabaChain kaynak kodunu indir
sudo -u babachain git clone https://github.com/Baba-Chain/BabaChain.git /home/babachain/BabaChain

# 5. Derle
sudo -u babachain bash << 'EOF'
cd /home/babachain/BabaChain
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
EOF

# 6. Binary'leri yükle
cd /home/babachain/BabaChain/build
make install

# 7. Konfigürasyon oluştur
sudo -u babachain mkdir -p /home/babachain/.babachain

# RPC şifresi oluştur
RPC_PASSWORD=$(openssl rand -base64 32)

# Konfigürasyon dosyası
sudo -u babachain tee /home/babachain/.babachain/babachain.conf > /dev/null << EOF
# BabaChain Seed Node Konfigürasyonu
rpcuser=babachain_seed
rpcpassword=$RPC_PASSWORD
rpcport=9998
rpcbind=0.0.0.0
rpcallowip=0.0.0.0/0

port=9999
listen=1
discover=1
upnp=1
maxconnections=200

staking=1
stakegen=1
reservebalance=100

timeout=5000
dbcache=512
maxmempool=500

debug=pos
debug=staking
debug=net

testnet=0
regtest=0

uacomment=BabaChain-Seed-Node
EOF

chmod 600 /home/babachain/.babachain/babachain.conf

# 8. Systemd servisi oluştur
tee /etc/systemd/system/babachaind.service > /dev/null << 'EOF'
[Unit]
Description=BabaChain daemon (seed node)
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

[Install]
WantedBy=multi-user.target
EOF

# 9. Servisi etkinleştir ve başlat
systemctl daemon-reload
systemctl enable babachaind
systemctl start babachaind

# 10. Firewall ayarları
ufw --force enable
ufw allow ssh
ufw allow 9999/tcp comment "BabaChain P2P"
ufw allow 9998/tcp comment "BabaChain RPC"

# 11. Durumu kontrol et
sleep 10
systemctl status babachaind

echo "✅ BabaChain seed node kurulumu tamamlandı!"
echo "🔑 RPC Şifresi: $RPC_PASSWORD"
echo "📍 IP Adresi: $(curl -s ifconfig.me)"
echo "🔌 P2P Port: 9999"
echo "🔧 RPC Port: 9998"
```

## 🔍 Kurulum Sonrası Kontroller

```bash
# Servis durumu
systemctl status babachaind

# Logları izle
journalctl -u babachaind -f

# BabaChain CLI komutları
sudo -u babachain babachain-cli getblockchaininfo
sudo -u babachain babachain-cli getnetworkinfo
sudo -u babachain babachain-cli getpeerinfo
```

## 🌐 macOS'tan Bağlanma

VPS kurulumu tamamlandıktan sonra, macOS'tan bağlanmak için:

```bash
# macOS'ta BabaChain kur
./scripts/macos-deploy.sh

# Konfigürasyona VPS IP'sini ekle
echo "addnode=VPS_IP_ADRESI:9999" >> ~/Library/Application\ Support/BabaChain/babachain.conf

# BabaChain'i yeniden başlat
launchctl stop org.babachain.babachaind
launchctl start org.babachain.babachaind

# Bağlantıyı kontrol et
babachain-cli getpeerinfo
```

## 🎯 Özet

**Sorun**: GitHub URL'i yanlış format
**Çözüm**: Repository'yi klonla ve script'i lokal çalıştır

```bash
# En basit çözüm:
git clone https://github.com/Baba-Chain/BabaChain.git
cd BabaChain
sudo ./scripts/vps-quick-setup.sh seed mainnet-seed1
```

Bu şekilde VPS'te seed node kurulur ve macOS'tan bağlanabilirsiniz! 🚀