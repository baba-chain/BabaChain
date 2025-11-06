# 🚀 BabaChain Docker - Hızlı Başlangıç

Bu rehber BabaChain'i Docker ile 5 dakikada çalıştırmanızı sağlar.

## ⚡ Tek Komutla Başlatma

```bash
# Proje dizininde
./docker/scripts/local-deploy.sh
```

Bu komut otomatik olarak:
- ✅ Docker kontrolü yapar
- ✅ Konfigürasyon oluşturur
- ✅ Image build eder
- ✅ Container'ı başlatır

## 📊 Durum Kontrolü

```bash
# Container durumu
./docker/scripts/manage.sh status

# Blockchain bilgisi
./docker/scripts/manage.sh info

# Staking durumu
./docker/scripts/manage.sh staking

# Wallet bilgisi
./docker/scripts/manage.sh wallet
```

## 🔧 Yönetim Komutları

```bash
# Logları görüntüle
./docker/scripts/manage.sh logs

# Container'a gir
./docker/scripts/manage.sh shell

# Canlı monitoring
./docker/scripts/manage.sh monitor

# Yeniden başlat
./docker/scripts/manage.sh restart

# Durdur
./docker/scripts/manage.sh stop

# Temizle (tüm veriyi siler!)
./docker/scripts/manage.sh clean
```

## 🌐 Bağlantı Bilgileri

Deployment sonrası:
- **P2P Port**: 19999
- **RPC Port**: 19998 (localhost only)
- **RPC Kullanıcı**: babachain_local
- **RPC Şifre**: Otomatik oluşturulur

## 🔌 RPC Test

```bash
# Curl ile test
curl -u babachain_local:YOUR_PASSWORD \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"1.0","id":"test","method":"getblockchaininfo","params":[]}' \
  http://127.0.0.1:19998/

# CLI ile test
docker exec babachain-local babachain-cli getblockchaininfo
```

## 🛠️ Troubleshooting

### Container başlamıyor
```bash
# Logları kontrol et
./docker/scripts/manage.sh logs

# Port çakışması kontrolü
netstat -tulpn | grep :19999
netstat -tulpn | grep :19998
```

### RPC bağlantı sorunu
```bash
# Container durumunu kontrol et
./docker/scripts/manage.sh status

# Container'a gir ve manuel test et
./docker/scripts/manage.sh shell
```

## 📱 Web Interface (Opsiyonel)

Basit web arayüzü için:

```bash
# Explorer başlat
docker run -d --name babachain-explorer \
  -p 8080:80 \
  -v $(pwd)/docker/explorer:/usr/share/nginx/html:ro \
  nginx:alpine

# Tarayıcıda aç: http://localhost:8080
```

## 🔄 Güncelleme

```bash
# Image'ı yeniden build et
./docker/scripts/manage.sh rebuild
```

## 🧹 Temizlik

```bash
# Sadece container'ı durdur
./docker/scripts/manage.sh stop

# Tüm veriyi sil (dikkatli!)
./docker/scripts/manage.sh clean
```

## 📚 Daha Fazla Bilgi

- **Detaylı Rehber**: [DOCKER_LOCAL_DEPLOYMENT.md](DOCKER_LOCAL_DEPLOYMENT.md)
- **Build Verification**: [contrib/devtools/README-build-verification.md](contrib/devtools/README-build-verification.md)

---

**🎉 Tebrikler! BabaChain Docker ile çalışıyor.**

Development modunda mock RPC responses kullanılır. Gerçek blockchain için production build gereklidir.