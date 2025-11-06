# 🐳 BabaChain Local Docker Deployment Rehberi

Bu rehber, BabaChain'i local Docker Desktop ortamında nasıl deploy edeceğinizi adım adım gösterir.

## 📋 Ön Gereksinimler

### 1. Docker Desktop Kurulumu
- [Docker Desktop](https://www.docker.com/products/docker-desktop/) indirin ve kurun
- Docker Desktop'ı başlatın ve çalıştığından emin olun

### 2. Sistem Gereksinimleri
- **RAM**: Minimum 4GB, önerilen 8GB+
- **Disk**: Minimum 20GB boş alan
- **CPU**: 2+ çekirdek önerilir

## 🚀 Hızlı Başlangıç

### Yöntem 1: Docker Compose ile (Önerilen)

```bash
# 1. Proje dizinine gidin
cd /path/to/BabaChain

# 2. Docker Compose ile başlatın
docker-compose -f docker/docker-compose.yml up -d

# 3. Logları kontrol edin
docker-compose -f docker/docker-compose.yml logs -f babachain
```

### Yöntem 2: Tek Container ile

```bash
# 1. Image'ı build edin
docker build -f docker/Dockerfile -t babachain:local .

# 2. Container'ı çalıştırın
docker run -d --name babachain-local \
  -p 9999:9999 \
  -p 9998:9998 \
  -v babachain-data:/home/babachain/.babachain \
  babachain:local
```

## 🔧 Detaylı Kurulum Adımları

### 1. Proje Hazırlığı

```bash
# Proje dizinine gidin
cd /path/to/BabaChain

# Docker dosyalarının varlığını kontrol edin
ls -la docker/
# Çıktı: Dockerfile, docker-compose.yml, docker-entrypoint.sh, README.md
```

### 2. Konfigürasyon Dosyası Oluşturma

```bash
# Konfigürasyon dizini oluşturun
mkdir -p docker/config

# Özel babachain.conf dosyası oluşturun
cat > docker/config/babachain.conf << EOF
# BabaChain Local Development Configuration

# RPC ayarları
rpcuser=babachain_local
rpcpassword=local_development_password_123
rpcport=9998
rpcbind=0.0.0.0
rpcallowip=127.0.0.1
rpcallowip=172.16.0.0/12

# Ağ ayarları
port=9999
listen=1
discover=1
upnp=0

# PoS staking ayarları
staking=1
stakegen=1
reservebalance=0

# Local development seed nodes
addnode=seed1.babachain.org:9999
addnode=seed2.babachain.org:9999

# Performans ayarları (local development için optimize)
maxconnections=50
timeout=5000
dbcache=200
maxmempool=100

# Debug ayarları
debug=pos,staking,net,rpc
printtoconsole=1

# Local development
testnet=0
regtest=0
EOF
```

### 3. Docker Compose Konfigürasyonu

Mevcut `docker/docker-compose.yml` dosyasını local development için optimize edelim:

```bash
# Local development için docker-compose override dosyası oluşturun
cat > docker/docker-compose.local.yml << EOF
version: '3.8'

services:
  babachain:
    build:
      context: ..
      dockerfile: docker/Dockerfile
    container_name: babachain-local
    restart: unless-stopped
    ports:
      - "9999:9999"  # P2P port
      - "127.0.0.1:9998:9998"  # RPC port (sadece localhost)
    volumes:
      - babachain-local-data:/home/babachain/.babachain
      - ./config/babachain.conf:/home/babachain/.babachain/babachain.conf:ro
      - ./logs:/home/babachain/logs
    environment:
      - RPC_USER=babachain_local
      - RPC_PASSWORD=local_development_password_123
      - STAKING=1
      - STAKEGEN=1
      - MAX_CONNECTIONS=50
      - DB_CACHE=200
      - DEBUG=pos,staking,net,rpc
      - TESTNET=0
    healthcheck:
      test: ["CMD", "babachain-cli", "-rpcuser=babachain_local", "-rpcpassword=local_development_password_123", "getblockcount"]
      interval: 30s
      timeout: 10s
      retries: 3
      start_period: 120s
    networks:
      - babachain-local-network

  # Local development için Adminer (veritabanı yönetimi)
  adminer:
    image: adminer:latest
    container_name: babachain-adminer
    restart: unless-stopped
    ports:
      - "8080:8080"
    networks:
      - babachain-local-network
    profiles:
      - tools

volumes:
  babachain-local-data:
    driver: local

networks:
  babachain-local-network:
    driver: bridge
EOF
```

### 4. Build ve Çalıştırma

```bash
# 1. Image'ı build edin
docker-compose -f docker/docker-compose.local.yml build

# 2. Container'ları başlatın
docker-compose -f docker/docker-compose.local.yml up -d

# 3. Başlatma durumunu kontrol edin
docker-compose -f docker/docker-compose.local.yml ps

# 4. Logları takip edin
docker-compose -f docker/docker-compose.local.yml logs -f babachain
```

## 📊 Monitoring ve Yönetim

### 1. Container Durumu Kontrolü

```bash
# Container'ların durumunu kontrol edin
docker ps

# Detaylı bilgi
docker stats babachain-local

# Health check durumu
docker inspect babachain-local | grep -A 10 "Health"
```

### 2. BabaChain CLI Komutları

```bash
# Blockchain bilgisi
docker exec babachain-local babachain-cli -rpcuser=babachain_local -rpcpassword=local_development_password_123 getblockchaininfo

# Staking bilgisi
docker exec babachain-local babachain-cli -rpcuser=babachain_local -rpcpassword=local_development_password_123 getstakinginfo

# Peer bağlantıları
docker exec babachain-local babachain-cli -rpcuser=babachain_local -rpcpassword=local_development_password_123 getpeerinfo

# Wallet bilgisi
docker exec babachain-local babachain-cli -rpcuser=babachain_local -rpcpassword=local_development_password_123 getwalletinfo

# Yeni adres oluşturma
docker exec babachain-local babachain-cli -rpcuser=babachain_local -rpcpassword=local_development_password_123 getnewaddress

# Balance kontrolü
docker exec babachain-local babachain-cli -rpcuser=babachain_local -rpcpassword=local_development_password_123 getbalance
```

### 3. Log Yönetimi

```bash
# Tüm logları görüntüle
docker-compose -f docker/docker-compose.local.yml logs babachain

# Son 100 satırı göster
docker-compose -f docker/docker-compose.local.yml logs --tail=100 babachain

# Canlı log takibi
docker-compose -f docker/docker-compose.local.yml logs -f babachain

# Belirli bir zaman aralığındaki loglar
docker-compose -f docker/docker-compose.local.yml logs --since="2024-01-01T00:00:00" babachain
```

## 🔧 Geliştirme Araçları

### 1. Development Shell

```bash
# Container içine bash ile girin
docker exec -it babachain-local bash

# Root olarak girin (gerekirse)
docker exec -it --user root babachain-local bash

# Belirli bir komut çalıştırın
docker exec babachain-local ls -la /home/babachain/.babachain/
```

### 2. Dosya Transferi

```bash
# Host'tan container'a dosya kopyalama
docker cp local_file.txt babachain-local:/home/babachain/

# Container'dan host'a dosya kopyalama
docker cp babachain-local:/home/babachain/.babachain/debug.log ./debug.log

# Wallet backup
docker cp babachain-local:/home/babachain/.babachain/wallet.dat ./wallet_backup.dat
```

### 3. Database Yönetimi

```bash
# Blockchain veritabanı boyutu
docker exec babachain-local du -sh /home/babachain/.babachain/blocks/

# Chainstate boyutu
docker exec babachain-local du -sh /home/babachain/.babachain/chainstate/

# Mempool durumu
docker exec babachain-local babachain-cli -rpcuser=babachain_local -rpcpassword=local_development_password_123 getmempoolinfo
```

## 🛠️ Troubleshooting

### 1. Yaygın Sorunlar ve Çözümleri

#### Container Başlamıyor
```bash
# Hata loglarını kontrol edin
docker-compose -f docker/docker-compose.local.yml logs babachain

# Port çakışması kontrolü
netstat -tulpn | grep :9999
netstat -tulpn | grep :9998

# Disk alanı kontrolü
df -h
docker system df
```

#### Blockchain Sync Sorunu
```bash
# Peer bağlantılarını kontrol edin
docker exec babachain-local babachain-cli -rpcuser=babachain_local -rpcpassword=local_development_password_123 getpeerinfo

# Manuel peer ekleme
docker exec babachain-local babachain-cli -rpcuser=babachain_local -rpcpassword=local_development_password_123 addnode "seed1.babachain.org:9999" "add"

# Sync durumu
docker exec babachain-local babachain-cli -rpcuser=babachain_local -rpcpassword=local_development_password_123 getblockchaininfo | grep -E "(blocks|headers|verificationprogress)"
```

#### RPC Bağlantı Sorunu
```bash
# RPC konfigürasyonunu kontrol edin
docker exec babachain-local cat /home/babachain/.babachain/babachain.conf

# RPC portunu test edin
curl -u babachain_local:local_development_password_123 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"1.0","id":"test","method":"getblockcount","params":[]}' \
  http://127.0.0.1:9998/
```

### 2. Performance Tuning

```bash
# Container kaynak kullanımını sınırlama
docker update --memory=2g --cpus=2 babachain-local

# Docker Compose ile kaynak sınırları
cat >> docker/docker-compose.local.yml << EOF
    deploy:
      resources:
        limits:
          memory: 2G
          cpus: '2'
        reservations:
          memory: 1G
          cpus: '1'
EOF
```

## 🔄 Güncelleme ve Bakım

### 1. Image Güncelleme

```bash
# Container'ı durdur
docker-compose -f docker/docker-compose.local.yml down

# Yeni image build et
docker-compose -f docker/docker-compose.local.yml build --no-cache

# Yeniden başlat
docker-compose -f docker/docker-compose.local.yml up -d
```

### 2. Veri Yedekleme

```bash
# Wallet yedekleme
docker exec babachain-local babachain-cli -rpcuser=babachain_local -rpcpassword=local_development_password_123 backupwallet /home/babachain/.babachain/wallet_backup.dat
docker cp babachain-local:/home/babachain/.babachain/wallet_backup.dat ./

# Tam veri yedekleme
docker run --rm -v babachain-local-data:/data -v $(pwd):/backup alpine tar czf /backup/babachain-backup-$(date +%Y%m%d).tar.gz /data
```

### 3. Temizlik

```bash
# Container'ları durdur ve kaldır
docker-compose -f docker/docker-compose.local.yml down

# Volume'ları da kaldır (DİKKAT: Tüm veri silinir!)
docker-compose -f docker/docker-compose.local.yml down -v

# Kullanılmayan Docker objelerini temizle
docker system prune -a
```

## 🌐 Web Interface (Opsiyonel)

### 1. Block Explorer

```bash
# Basit block explorer için
cat > docker/docker-compose.explorer.yml << EOF
version: '3.8'

services:
  explorer:
    image: nginx:alpine
    container_name: babachain-explorer
    ports:
      - "8081:80"
    volumes:
      - ./explorer:/usr/share/nginx/html:ro
    networks:
      - babachain-local-network

networks:
  babachain-local-network:
    external: true
EOF

# Explorer dosyalarını oluştur
mkdir -p docker/explorer
cat > docker/explorer/index.html << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>BabaChain Local Explorer</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .info-box { background: #f5f5f5; padding: 20px; margin: 10px 0; border-radius: 5px; }
        button { padding: 10px 20px; margin: 5px; background: #007cba; color: white; border: none; border-radius: 3px; cursor: pointer; }
        button:hover { background: #005a87; }
        #result { background: #f9f9f9; padding: 15px; margin: 10px 0; border-left: 4px solid #007cba; white-space: pre-wrap; }
    </style>
</head>
<body>
    <h1>🔗 BabaChain Local Explorer</h1>
    
    <div class="info-box">
        <h3>Node Information</h3>
        <button onclick="getInfo()">Get Node Info</button>
        <button onclick="getBlockchainInfo()">Blockchain Info</button>
        <button onclick="getStakingInfo()">Staking Info</button>
        <button onclick="getPeerInfo()">Peer Info</button>
    </div>
    
    <div id="result"></div>
    
    <script>
        const rpcCall = async (method, params = []) => {
            try {
                const response = await fetch('http://127.0.0.1:9998/', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                        'Authorization': 'Basic ' + btoa('babachain_local:local_development_password_123')
                    },
                    body: JSON.stringify({
                        jsonrpc: '1.0',
                        id: 'explorer',
                        method: method,
                        params: params
                    })
                });
                
                const data = await response.json();
                document.getElementById('result').textContent = JSON.stringify(data.result, null, 2);
            } catch (error) {
                document.getElementById('result').textContent = 'Error: ' + error.message;
            }
        };
        
        const getInfo = () => rpcCall('getinfo');
        const getBlockchainInfo = () => rpcCall('getblockchaininfo');
        const getStakingInfo = () => rpcCall('getstakinginfo');
        const getPeerInfo = () => rpcCall('getpeerinfo');
    </script>
</body>
</html>
EOF

# Explorer'ı başlat
docker-compose -f docker/docker-compose.explorer.yml up -d
```

## 📱 Mobil Erişim (Local Network)

Local ağınızdaki diğer cihazlardan erişim için:

```bash
# Host IP adresini öğrenin
hostname -I

# Docker Compose'u external IP ile başlatın
cat > docker/docker-compose.mobile.yml << EOF
version: '3.8'

services:
  babachain:
    extends:
      file: docker-compose.local.yml
      service: babachain
    ports:
      - "0.0.0.0:9999:9999"  # Tüm interface'lere bind
      - "0.0.0.0:9998:9998"  # DİKKAT: Güvenlik riski!
    environment:
      - RPC_BIND=0.0.0.0
      - RPC_ALLOWIP=192.168.0.0/16,10.0.0.0/8,172.16.0.0/12
EOF

# Güvenlik uyarısı ile başlat
echo "⚠️  UYARI: RPC portu tüm ağa açık! Sadece güvenli ağlarda kullanın."
docker-compose -f docker/docker-compose.mobile.yml up -d
```

Bu rehber ile BabaChain'i local Docker Desktop ortamında başarıyla deploy edebilir ve yönetebilirsiniz. Herhangi bir sorunla karşılaştığınızda troubleshooting bölümünü kontrol edin.