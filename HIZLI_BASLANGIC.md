# 🚀 BabaChain Hızlı Başlangıç Rehberi

## 3 Farklı Kurulum Yöntemi

### 🐳 Yöntem 1: Docker ile Kurulum (EN KOLAY)

```bash
# 1. Repository'yi klonla
git clone https://github.com/BabaChain/BabaChain.git
cd BabaChain

# 2. Docker Compose ile başlat
cd docker
docker-compose up -d

# 3. Durumu kontrol et
docker-compose logs -f babachain

# 4. Cüzdan oluştur
docker-compose exec babachain babachain-cli createwallet "ana_cuzdan"

# 5. Adres oluştur
docker-compose exec babachain babachain-cli getnewaddress
```

### 🖥️ Yöntem 2: Ubuntu Otomatik Kurulum

```bash
# 1. Repository'yi klonla
git clone https://github.com/BabaChain/BabaChain.git
cd BabaChain

# 2. Otomatik kurulum scriptini çalıştır
chmod +x scripts/ubuntu-deploy.sh
./scripts/ubuntu-deploy.sh

# 3. Kurulum tamamlandıktan sonra
sudo su - babachain
babachain-cli getblockchaininfo
```

### 🔧 Yöntem 3: Manuel Kurulum

```bash
# 1. Sistem güncellemesi
sudo apt update && sudo apt upgrade -y

# 2. Gerekli paketleri yükle
sudo apt install -y build-essential cmake git libboost-all-dev libssl-dev

# 3. BabaChain'i derle
git clone https://github.com/BabaChain/BabaChain.git
cd BabaChain
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install

# 4. Konfigürasyon oluştur
mkdir -p ~/.babachain
# Konfigürasyon dosyasını oluştur (detaylar BABACHAIN_UBUNTU_DEPLOYMENT.md'de)

# 5. Başlat
babachaind -daemon
```

## ⚡ Hızlı Komutlar

### Temel Komutlar
```bash
# Blockchain durumu
babachain-cli getblockchaininfo

# Ağ durumu  
babachain-cli getnetworkinfo

# Staking durumu
babachain-cli getstakinginfo

# Bakiye kontrolü
babachain-cli getbalance

# Yeni adres oluştur
babachain-cli getnewaddress

# İşlem gönder
babachain-cli sendtoaddress "ADRES" MIKTAR
```

### Staking Komutları
```bash
# Cüzdanı staking için aç
babachain-cli walletpassphrase "ŞİFRE" 999999999 true

# Staking durumunu kontrol et
babachain-cli getstakinginfo

# Staking ödüllerini görüntüle
babachain-cli listtransactions "*" 10 0 true
```

### Sistem Komutları (Ubuntu)
```bash
# Servis durumu
sudo systemctl status babachaind

# Logları izle
sudo journalctl -u babachaind -f

# Servisi yeniden başlat
sudo systemctl restart babachaind
```

## 🎯 İlk Adımlar

### 1. Cüzdan Kurulumu
```bash
# Yeni cüzdan oluştur
babachain-cli createwallet "ana_cuzdan"

# Cüzdanı şifrele
babachain-cli encryptwallet "GÜÇLÜ_ŞİFRE"

# Yedekleme için seed phrase al
babachain-cli dumpwallet "cuzdan_yedek.txt"
```

### 2. İlk BabaChain Alma
```bash
# Yeni adres oluştur
ADRES=$(babachain-cli getnewaddress)
echo "BabaChain adresiniz: $ADRES"

# Bu adrese BabaChain gönderin (exchange'den veya başka cüzdandan)
# Bakiyeyi kontrol edin
babachain-cli getbalance
```

### 3. Staking'e Başlama
```bash
# Cüzdanı staking için aç
babachain-cli walletpassphrase "ŞİFRE" 999999999 true

# Staking durumunu kontrol et
babachain-cli getstakinginfo

# Staking başladığında şu bilgileri göreceksiniz:
# - "enabled": true
# - "staking": true  
# - "expectedtime": tahmini süre
```

## 📊 Beklenen Getiriler

### Staking Ödülleri
- **İlk 20M coin**: 200 BabaChain/blok
- **20M-40M coin**: 150 BabaChain/blok  
- **40M-60M coin**: 100 BabaChain/blok
- **Blok süresi**: 2.5 dakika
- **Yıllık getiri**: %25-365+ (ağ katılımına bağlı)

### ROI Hesaplaması
```
Günlük blok sayısı: 576 blok (24 saat / 2.5 dakika)
Aylık blok sayısı: ~17,280 blok
Yıllık blok sayısı: ~210,240 blok

Örnek: 1000 BabaChain ile staking
- Ağ katılımı %30 ise: ~%25 yıllık getiri
- Ağ katılımı %15 ise: ~%65 yıllık getiri  
- Ağ katılımı %10 ise: ~%120 yıllık getiri
```

## 🔧 Sorun Giderme

### Yaygın Sorunlar

#### Daemon başlamıyor
```bash
# Logları kontrol et
tail -f ~/.babachain/debug.log

# Port kullanımını kontrol et
sudo netstat -tulpn | grep 9999

# Konfigürasyonu kontrol et
cat ~/.babachain/babachain.conf
```

#### Senkronizasyon yavaş
```bash
# Peer sayısını kontrol et
babachain-cli getconnectioncount

# Manuel peer ekle
babachain-cli addnode "seed1.babachain.org:9999" "add"
```

#### Staking çalışmıyor
```bash
# Cüzdan kilidini kontrol et
babachain-cli walletinfo

# Bakiyeyi kontrol et (minimum 1 BabaChain gerekli)
babachain-cli getbalance

# Cüzdanı staking için aç
babachain-cli walletpassphrase "ŞİFRE" 999999999 true
```

## 🌐 Topluluk ve Destek

### Resmi Kanallar
- **Website**: https://babachain.org
- **GitHub**: https://github.com/BabaChain/BabaChain
- **Discord**: https://discord.gg/babachain
- **Telegram**: https://t.me/babachainofficial
- **Twitter**: https://twitter.com/BabaChainOrg

### Türkçe Topluluk
- **Telegram TR**: https://t.me/babachainturkiye
- **Discord TR**: https://discord.gg/babachain-tr

## 🎁 Airdrop ve Ödüller

### Genesis Airdrop
- **Toplam**: 1M BabaChain
- **Koşullar**: Discord/Telegram katılımı, sosyal medya takibi
- **Bonus**: Erken katılım, referans, staking bonusları

### Referans Programı
- **Referans bonusu**: %5 lifetime staking ödülleri
- **Referee bonusu**: İlk ay %10 extra staking
- **Tier sistemi**: Bronze, Silver, Gold, Platinum

## 📈 Yol Haritası

### 2025 Q4 (Ekim-Aralık)
- ✅ Mainnet lansmanı (1 Aralık 2025)
- 🔄 Exchange listelemeleri
- 🔄 Mobile wallet lansmanı
- 🔄 DeFi entegrasyonları

### 2026 Q1 (Ocak-Mart)  
- 🔄 NFT marketplace
- 🔄 Governance sistemi
- 🔄 Cross-chain köprüleri
- 🔄 Developer grants programı

---

**Başarılı staking'ler! 🚀**

*Bu rehber sürekli güncellenmektedir. En son versiyonu için GitHub'ı takip edin.*