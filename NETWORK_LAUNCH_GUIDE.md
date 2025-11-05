# 🚀 BabaChain Network Launch Rehberi

## 🎯 Ağ Lansmanı Süreci

BabaChain ağını ilk defa başlatmak için belirli bir sıra takip etmemiz gerekiyor. Bu rehber size adım adım ne yapmanız gerektiğini gösterir.

## 📅 Lansman Tarihleri
- **Testnet**: Kasım 2025
- **Mainnet**: 1 Ocak 2026, 00:00:00 UTC

## 🔄 Lansman Aşamaları

### Aşama 1: Hazırlık (Lansmandan 30 gün önce)

#### 1.1 Seed Node'ları Hazırlama
```bash
# Seed node'ları deploy et
./scripts/setup-seed-nodes.py

# DNS kayıtlarını güncelle
# seed1.babachain.org -> IP_ADDRESS_1
# seed2.babachain.org -> IP_ADDRESS_2
# seed3.babachain.org -> IP_ADDRESS_3
# node1.babachain.network -> IP_ADDRESS_4
# node2.babachain.network -> IP_ADDRESS_5
```

#### 1.2 Genesis Block Hazırlama
```bash
# Genesis block parametrelerini hazırla
python3 scripts/prepare-mainnet.py

# Genesis block'u oluştur
./scripts/generate-genesis.sh
```

#### 1.3 Network Parametrelerini Doğrula
```bash
# Chainparams'ı kontrol et
grep -n "1764547200" src/chainparams.cpp  # 1 Aralık 2025 timestamp
grep -n "20000000 \* COIN" src/chainparams.cpp  # 20M premine
grep -n "0xbaba1337" src/chainparams.cpp  # Network magic
```

### Aşama 2: Testnet Lansmanı (Kasım 2025)

#### 2.1 Testnet Seed Node'larını Başlat
```bash
# Testnet konfigürasyonu ile başlat
babachaind -testnet -daemon

# Testnet genesis block'unu oluştur
babachain-cli -testnet getblockhash 0
```

#### 2.2 Testnet Validasyonu
```bash
# Network durumunu kontrol et
babachain-cli -testnet getnetworkinfo
babachain-cli -testnet getblockchaininfo
babachain-cli -testnet getstakinginfo

# Peer bağlantılarını kontrol et
babachain-cli -testnet getpeerinfo
```

### Aşama 3: Mainnet Lansmanı (1 Aralık 2025)

#### 3.1 Lansman Öncesi Son Kontroller (T-24 saat)

```bash
# Tüm seed node'ların hazır olduğunu kontrol et
for node in seed1.babachain.org seed2.babachain.org seed3.babachain.org; do
    echo "Testing $node..."
    nc -zv $node 9999
done

# Genesis block hash'ini doğrula
python3 -c "
import hashlib
import struct
from datetime import datetime

# Genesis block parametreleri
timestamp = 1764547200  # 1 Aralık 2025
premine = 20000000 * 100000000  # 20M BabaChain (satoshi cinsinden)

print(f'Genesis timestamp: {timestamp}')
print(f'Genesis date: {datetime.fromtimestamp(timestamp)}')
print(f'Premine amount: {premine / 100000000} BabaChain')
"
```

#### 3.2 Koordineli Lansman (T-0)

**Saat 00:00:00 UTC'de sırayla:**

1. **İlk Seed Node'u Başlat (seed1.babachain.org)**
```bash
# İlk node'u başlat
babachaind -daemon -listen=1 -discover=1

# Genesis block'un oluştuğunu kontrol et
sleep 10
babachain-cli getblockhash 0
babachain-cli getblock $(babachain-cli getblockhash 0)
```

2. **Diğer Seed Node'ları Başlat (30 saniye arayla)**
```bash
# Her seed node için
babachaind -daemon -addnode=seed1.babachain.org:9999

# Bağlantıyı kontrol et
babachain-cli getpeerinfo
```

3. **Network Senkronizasyonunu Doğrula**
```bash
# Tüm node'ların aynı genesis hash'e sahip olduğunu kontrol et
for node in seed1 seed2 seed3 node1 node2; do
    echo "Checking $node..."
    # SSH ile bağlan ve kontrol et
    ssh $node "babachain-cli getblockhash 0"
done
```

### Aşama 4: Lansman Sonrası (T+1 saat)

#### 4.1 Network Health Check
```bash
# Network sağlığını kontrol et
python3 scripts/network-autoscaling.py

# Staking durumunu kontrol et
babachain-cli getstakinginfo

# İlk blokların üretildiğini kontrol et
babachain-cli getblockcount
```

#### 4.2 Wallet'ları Aktif Et
```bash
# Desktop wallet'ları release et
# Mobile wallet'ları app store'lara yükle
# Web wallet'ı aktif et
```

#### 4.3 Exchange Entegrasyonları
```bash
# Exchange'lere network parametrelerini gönder
# API endpoint'lerini aktif et
# Trading'i başlat
```

## 🛠️ Lansman Checklist'i

### Teknik Hazırlık ✅
- [ ] Seed node'lar deploy edildi
- [ ] DNS kayıtları güncellendi
- [ ] Genesis block parametreleri doğrulandı
- [ ] Network magic bytes ayarlandı
- [ ] Firewall kuralları yapılandırıldı
- [ ] SSL sertifikaları kuruldu
- [ ] Monitoring sistemleri aktif
- [ ] Backup sistemleri hazır

### Testnet Validasyonu ✅
- [ ] Testnet başarıyla çalıştı
- [ ] Staking test edildi
- [ ] Peer discovery çalışıyor
- [ ] RPC interface'ler test edildi
- [ ] Mobile wallet'lar test edildi
- [ ] Performance test'leri geçti

### Mainnet Lansmanı ✅
- [ ] Genesis timestamp doğru (1764547200)
- [ ] Premine amount doğru (20M BabaChain)
- [ ] Seed node'lar senkronize
- [ ] İlk blok üretildi
- [ ] Staking aktif
- [ ] Wallet'lar çalışıyor

### Lansman Sonrası ✅
- [ ] Network health %100
- [ ] Exchange listelemeleri aktif
- [ ] Marketing kampanyaları başladı
- [ ] Community support aktif
- [ ] Documentation güncel

## 🚨 Acil Durum Planı

### Lansman Sırasında Sorun Çıkarsa

#### Senaryo 1: Genesis Block Sorunu
```bash
# Tüm node'ları durdur
systemctl stop babachaind

# Genesis parametrelerini düzelt
# Chainparams'ı yeniden derle
make -j$(nproc)

# Node'ları yeniden başlat
systemctl start babachaind
```

#### Senaryo 2: Network Bölünmesi
```bash
# En uzun chain'i belirle
babachain-cli getblockchaininfo

# Diğer node'ları doğru chain'e yönlendir
babachain-cli addnode "CORRECT_NODE_IP:9999" "add"
```

#### Senaryo 3: Staking Sorunu
```bash
# Staking durumunu kontrol et
babachain-cli getstakinginfo

# Cüzdan kilidini kontrol et
babachain-cli walletinfo

# Gerekirse cüzdanı yeniden aç
babachain-cli walletpassphrase "PASSWORD" 999999999 true
```

## 📊 Lansman Metrikleri

### Başarı Kriterleri
- **Network Uptime**: %99.9+
- **Seed Node Availability**: 5/5 aktif
- **Block Time**: 150 saniye (±10%)
- **Peer Connections**: 50+ peer
- **Staking Participation**: %10+ ilk gün

### İzlenecek Metrikler
```bash
# Network durumu
babachain-cli getnetworkinfo | jq '.connections'

# Blockchain durumu  
babachain-cli getblockchaininfo | jq '.blocks'

# Staking durumu
babachain-cli getstakinginfo | jq '.enabled'

# Memory/CPU kullanımı
ps aux | grep babachaind
```

## 🎉 Lansman Sonrası Aktiviteler

### İlk 24 Saat
1. **Network monitoring** - Sürekli izleme
2. **Community support** - Discord/Telegram aktif destek
3. **Exchange coordination** - Trading başlatma
4. **Marketing activation** - Sosyal medya kampanyaları

### İlk Hafta
1. **Performance optimization** - Network ayarları
2. **Bug fixes** - Acil düzeltmeler
3. **User onboarding** - Yeni kullanıcı desteği
4. **Partnership activation** - İş ortaklıkları

### İlk Ay
1. **Feature rollout** - Yeni özellikler
2. **Ecosystem growth** - DeFi entegrasyonları
3. **Community events** - AMA'lar, yarışmalar
4. **Global expansion** - Uluslararası pazarlara giriş

## 🔗 Kritik Linkler

### Teknik Dokümantasyon
- Genesis Block Spec: `config/mainnet_genesis.json`
- Network Parameters: `src/chainparams.cpp`
- Seed Node Config: `config/seed-nodes/`

### Monitoring Dashboards
- Network Health: `http://monitor.babachain.org`
- Block Explorer: `http://explorer.babachain.org`
- API Status: `http://api.babachain.org/status`

### Communication Channels
- **Emergency**: Telegram @BabaChainEmergency
- **Technical**: Discord #technical-support
- **Community**: Telegram @BabaChainOfficial

---

**🎯 Başarılı bir lansman için bu rehberi adım adım takip edin!**

**⚠️ Önemli**: Lansman sırasında tüm ekip üyelerinin hazır olması ve koordineli hareket etmesi kritiktir.