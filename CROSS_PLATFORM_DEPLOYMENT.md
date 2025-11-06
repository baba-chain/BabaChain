# 🌐 BabaChain Cross-Platform Deployment Rehberi

## 🎯 Genel Bakış

BabaChain artık **Ubuntu Linux**, **macOS** ve **Windows** işletim sistemlerinde çalışabilir! Bu rehber size hangi platformda nasıl kurulum yapacağınızı gösterir.

**🗓️ Mainnet Lansmanı: 1 Aralık 2025**

## 🖥️ Desteklenen Platformlar

### 🐧 Ubuntu Linux
- **Minimum**: Ubuntu 20.04 LTS
- **Önerilen**: Ubuntu 22.04 LTS veya üzeri
- **RAM**: 8 GB (16 GB önerilen)
- **Disk**: 100 GB SSD

### 🍎 macOS
- **Minimum**: macOS 10.15 Catalina
- **Önerilen**: macOS 13.0 Ventura veya üzeri
- **Desteklenen**: Intel ve Apple Silicon (M1/M2/M3)
- **RAM**: 8 GB (16 GB önerilen)
- **Disk**: 100 GB SSD

### 🪟 Windows
- **Minimum**: Windows 10 (64-bit)
- **Önerilen**: Windows 11 Pro
- **RAM**: 8 GB (16 GB önerilen)
- **Disk**: 100 GB SSD

## 🚀 Hızlı Kurulum

### Ubuntu Linux - Otomatik Kurulum
```bash
# Repository'yi klonla
git clone https://github.com/Baba-Chain/BabaChain.git
cd BabaChain

# Otomatik kurulum scriptini çalıştır
chmod +x scripts/ubuntu-deploy.sh
./scripts/ubuntu-deploy.sh
```

### macOS - Otomatik Kurulum
```bash
# Repository'yi klonla
git clone https://github.com/Baba-Chain/BabaChain.git
cd BabaChain

# Otomatik kurulum scriptini çalıştır
chmod +x scripts/macos-deploy.sh
./scripts/macos-deploy.sh
```

### Windows - Otomatik Kurulum
```powershell
# PowerShell'i yönetici olarak açın
# Repository'yi klonla
git clone https://github.com/Baba-Chain/BabaChain.git
cd BabaChain

# Otomatik kurulum scriptini çalıştır
.\scripts\windows-deploy.ps1
```

## 📚 Detaylı Rehberler

| Platform | Detaylı Rehber | Otomatik Script |
|----------|----------------|-----------------|
| 🐧 Ubuntu | [BABACHAIN_UBUNTU_DEPLOYMENT.md](BABACHAIN_UBUNTU_DEPLOYMENT.md) | `scripts/ubuntu-deploy.sh` |
| 🍎 macOS | [BABACHAIN_MACOS_DEPLOYMENT.md](BABACHAIN_MACOS_DEPLOYMENT.md) | `scripts/macos-deploy.sh` |
| 🪟 Windows | [BABACHAIN_WINDOWS_DEPLOYMENT.md](BABACHAIN_WINDOWS_DEPLOYMENT.md) | `scripts/windows-deploy.ps1` |

## 🔧 Platform-Specific Özellikler

### Ubuntu Linux
- ✅ Systemd servisi olarak çalışır
- ✅ UFW firewall entegrasyonu
- ✅ Fail2ban güvenlik desteği
- ✅ Cron ile otomatik yedekleme
- ✅ Journalctl log yönetimi

### macOS
- ✅ LaunchAgent servisi olarak çalışır
- ✅ Homebrew paket yöneticisi
- ✅ Apple Silicon (M1/M2/M3) desteği
- ✅ macOS Firewall entegrasyonu
- ✅ GUI wallet (babachain-qt) desteği

### Windows
- ✅ Windows Service olarak çalışır
- ✅ NSSM service manager
- ✅ PowerShell aliasları
- ✅ Task Scheduler entegrasyonu
- ✅ Event Viewer log desteği
- ✅ GUI wallet (babachain-qt) desteği

## 🎮 Temel Komutlar (Tüm Platformlar)

### Blockchain Durumu
```bash
# Ubuntu/macOS
babachain-cli getblockchaininfo

# Windows
babachain-cli.exe getblockchaininfo
```

### Staking Durumu
```bash
# Ubuntu/macOS
babachain-cli getstakinginfo

# Windows
babachain-cli.exe getstakinginfo
```

### Cüzdan İşlemleri
```bash
# Yeni cüzdan oluştur
babachain-cli createwallet "ana_cuzdan"

# Yeni adres oluştur
babachain-cli getnewaddress

# Bakiye kontrol et
babachain-cli getbalance
```

## 🔄 Servis Yönetimi

### Ubuntu Linux
```bash
# Servis durumu
sudo systemctl status babachaind

# Servisi başlat/durdur/yeniden başlat
sudo systemctl start babachaind
sudo systemctl stop babachaind
sudo systemctl restart babachaind

# Logları izle
sudo journalctl -u babachaind -f
```

### macOS
```bash
# LaunchAgent durumu
launchctl list | grep babachain

# Servisi başlat/durdur
launchctl start org.babachain.babachaind
launchctl stop org.babachain.babachaind

# Logları izle
tail -f ~/Library/Logs/BabaChain/babachaind.log
```

### Windows
```powershell
# Servis durumu
Get-Service BabaChain

# Servisi başlat/durdur/yeniden başlat
Start-Service BabaChain
Stop-Service BabaChain
Restart-Service BabaChain

# Event logları
Get-EventLog -LogName Application -Source BabaChain
```

## 💰 Staking Getiri Hesaplaması

### Beklenen Yıllık Getiriler
- **Ağ katılımı %30**: ~%25 yıllık getiri
- **Ağ katılımı %20**: ~%45 yıllık getiri
- **Ağ katılımı %15**: ~%65 yıllık getiri
- **Ağ katılımı %10**: ~%120 yıllık getiri

### Ödül Programı
| Coin Aralığı | Blok Ödülü | Tahmini Süre |
|--------------|-------------|--------------|
| 0-20M | 200 BabaChain | ~3 ay |
| 20M-40M | 150 BabaChain | ~4 ay |
| 40M-60M | 100 BabaChain | ~6 ay |
| 60M-80M | 75 BabaChain | ~8 ay |
| 80M-100M | 50 BabaChain | ~12 ay |

## 🛡️ Güvenlik Önerileri

### Tüm Platformlar
1. **Güçlü şifreler kullanın**
2. **Düzenli yedekleme yapın**
3. **Firewall'ı etkinleştirin**
4. **Sistem güncellemelerini takip edin**
5. **Cüzdan dosyalarını şifreleyin**

### Platform-Specific Güvenlik
- **Ubuntu**: UFW firewall + Fail2ban
- **macOS**: macOS Firewall + FileVault
- **Windows**: Windows Defender + BitLocker

## 🔧 Sorun Giderme

### Yaygın Sorunlar

#### Daemon Başlamıyor
```bash
# Log dosyalarını kontrol edin
# Ubuntu: sudo journalctl -u babachaind -f
# macOS: tail -f ~/Library/Logs/BabaChain/babachaind.log
# Windows: Get-EventLog -LogName Application -Source BabaChain
```

#### Senkronizasyon Yavaş
```bash
# Peer sayısını kontrol edin
babachain-cli getconnectioncount

# Manuel peer ekleyin
babachain-cli addnode "seed1.babachain.org:9999" "add"
```

#### Staking Çalışmıyor
```bash
# Cüzdan kilidini kontrol edin
babachain-cli walletinfo

# Cüzdanı staking için açın
babachain-cli walletpassphrase "ŞİFRE" 999999999 true
```

## 📊 Performans Karşılaştırması

| Özellik | Ubuntu | macOS | Windows |
|---------|--------|-------|---------|
| **Kurulum Kolaylığı** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ |
| **Performans** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| **Güvenlik** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ |
| **Kaynak Kullanımı** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ |
| **GUI Desteği** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |

## 🌟 Önerilen Kurulum

### Sunucu/Mining için: Ubuntu Linux
- En iyi performans
- En düşük kaynak kullanımı
- 7/24 çalışma için optimize
- Uzaktan yönetim kolaylığı

### Desktop/Günlük kullanım için: macOS/Windows
- GUI wallet desteği
- Kullanıcı dostu arayüz
- Desktop entegrasyonu
- Kolay yönetim

## 🎯 Sonuç

BabaChain artık tüm major platformlarda çalışıyor! Hangi işletim sistemini kullanırsanız kullanın, BabaChain staking'e katılabilir ve ağa katkıda bulunabilirsiniz.

**🗓️ Önemli Tarihler:**
- **Testnet**: Kasım 2025
- **Mainnet**: 1 Aralık 2025

**🔗 Destek:**
- GitHub: https://github.com/Baba-Chain/BabaChain
- Discord: https://discord.gg/babachain
- Telegram: https://t.me/babachainofficial

---

**Başarılı staking'ler! 🚀**