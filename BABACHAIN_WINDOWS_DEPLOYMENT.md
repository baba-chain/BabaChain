# 🪟 BabaChain Windows Deployment Rehberi

## Genel Bakış
Bu rehber BabaChain blockchain'ini Windows üzerinde nasıl kuracağınızı ve çalıştıracağınızı gösterir.

## 📋 Sistem Gereksinimleri

### Minimum Gereksinimler
- **Windows**: 10 (64-bit) veya üzeri
- **RAM**: 8 GB (16 GB önerilen)
- **Disk**: 100 GB boş alan (SSD önerilen)
- **CPU**: Intel/AMD 64-bit işlemci
- **Visual Studio Build Tools**: Gerekli

### Önerilen Konfigürasyon
- **Windows**: 11 Pro
- **RAM**: 32 GB
- **Disk**: 500 GB NVMe SSD
- **CPU**: Intel i7/AMD Ryzen 7 veya üzeri

## 🛠️ Kurulum Adımları

### 1. Visual Studio Build Tools Kurulumu
```powershell
# PowerShell'i yönetici olarak açın

# Chocolatey yükle (paket yöneticisi)
Set-ExecutionPolicy Bypass -Scope Process -Force
[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))

# Visual Studio Build Tools yükle
choco install visualstudio2022buildtools -y
choco install visualstudio2022-workload-vctools -y
```

### 2. Git ve Gerekli Araçları Yükleme
```powershell
# Git yükle
choco install git -y

# CMake yükle
choco install cmake -y

# Python yükle
choco install python -y

# 7-Zip yükle
choco install 7zip -y

# PowerShell'i yeniden başlatın
```

### 3. vcpkg ile Bağımlılıkları Yükleme
```powershell
# vcpkg'yi klonla
cd C:\
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# vcpkg'yi derle
.\bootstrap-vcpkg.bat

# Gerekli paketleri yükle
.\vcpkg install boost:x64-windows
.\vcpkg install openssl:x64-windows
.\vcpkg install berkeleydb:x64-windows
.\vcpkg install zeromq:x64-windows
.\vcpkg install miniupnpc:x64-windows
.\vcpkg install protobuf:x64-windows
.\vcpkg install qt5:x64-windows
.\vcpkg install libevent:x64-windows
.\vcpkg install qrencode:x64-windows

# vcpkg'yi global olarak entegre et
.\vcpkg integrate install
```

### 4. BabaChain Kaynak Kodunu İndirme
```powershell
# Ana dizine git
cd C:\

# BabaChain repository'sini klonla
git clone https://github.com/Baba-Chain/BabaChain.git
cd BabaChain

# En son stable branch'e geç
git checkout mainnet-v1.0
```

### 5. BabaChain Derleme
```powershell
# Derleme dizinini oluştur
mkdir build
cd build

# CMake ile yapılandır
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=Release

# Derle (CPU çekirdek sayısına göre ayarla)
cmake --build . --config Release --parallel

# Binary'leri kopyala
copy Release\babachaind.exe C:\Windows\System32\
copy Release\babachain-cli.exe C:\Windows\System32\
copy Release\babachain-qt.exe C:\Windows\System32\
```

### 6. Konfigürasyon Dosyası Oluşturma
```powershell
# BabaChain veri dizinini oluştur
$dataDir = "$env:APPDATA\BabaChain"
New-Item -ItemType Directory -Force -Path $dataDir

# Konfigürasyon dosyasını oluştur
$configContent = @"
# BabaChain Windows Konfigürasyonu
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

# Windows optimizasyonları
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
"@

$configContent | Out-File -FilePath "$dataDir\babachain.conf" -Encoding UTF8
```

### 7. Windows Servisi Oluşturma
```powershell
# NSSM (Non-Sucking Service Manager) yükle
choco install nssm -y

# BabaChain servisini oluştur
nssm install BabaChain "C:\Windows\System32\babachaind.exe"
nssm set BabaChain Parameters "-daemon"
nssm set BabaChain DisplayName "BabaChain Daemon"
nssm set BabaChain Description "BabaChain Proof-of-Stake Cryptocurrency Node"
nssm set BabaChain Start SERVICE_AUTO_START

# Servisi başlat
nssm start BabaChain
```

## 🚀 BabaChain'i Başlatma

### 1. GUI Wallet ile Başlatma
```powershell
# BabaChain Qt wallet'ı başlat
babachain-qt.exe
```

### 2. Komut Satırı ile Başlatma
```powershell
# BabaChain daemon'unu başlat
babachaind.exe -daemon

# Durumu kontrol et
babachain-cli.exe getblockchaininfo
```

### 3. Windows Servisi ile Başlatma
```powershell
# Servisi başlat
Start-Service BabaChain

# Servis durumunu kontrol et
Get-Service BabaChain

# Servisi durdur
Stop-Service BabaChain
```

### 4. Cüzdan Oluşturma ve Staking
```powershell
# Yeni cüzdan oluştur
babachain-cli.exe createwallet "ana_cuzdan"

# Yeni adres oluştur
babachain-cli.exe getnewaddress

# Cüzdanı şifrele
babachain-cli.exe encryptwallet "GÜÇLÜ_CÜZDAN_ŞİFRESİ"

# Staking için cüzdanı aç
babachain-cli.exe walletpassphrase "GÜÇLÜ_CÜZDAN_ŞİFRESİ" 999999999 $true

# Staking durumunu kontrol et
babachain-cli.exe getstakinginfo
```

## 🔧 Günlük İşlemler

### Sistem Durumu Kontrolü
```powershell
# Node durumu
babachain-cli.exe getinfo

# Blockchain durumu
babachain-cli.exe getblockchaininfo

# Staking durumu
babachain-cli.exe getstakinginfo

# Cüzdan bakiyesi
babachain-cli.exe getbalance

# Peer bağlantıları
babachain-cli.exe getpeerinfo
```

### Log İzleme
```powershell
# BabaChain debug logları
Get-Content "$env:APPDATA\BabaChain\debug.log" -Tail 50 -Wait

# Sadece staking logları
Select-String -Path "$env:APPDATA\BabaChain\debug.log" -Pattern "staking"
```

### Servis Yönetimi
```powershell
# Servisi durdur
Stop-Service BabaChain

# Servisi başlat
Start-Service BabaChain

# Servisi yeniden başlat
Restart-Service BabaChain

# Servis durumu
Get-Service BabaChain
```

## 🛡️ Güvenlik Ayarları

### 1. Windows Firewall Konfigürasyonu
```powershell
# Windows Firewall kuralları ekle
New-NetFirewallRule -DisplayName "BabaChain P2P" -Direction Inbound -Protocol TCP -LocalPort 9999 -Action Allow
New-NetFirewallRule -DisplayName "BabaChain RPC" -Direction Inbound -Protocol TCP -LocalPort 9998 -Action Allow -RemoteAddress LocalSubnet
```

### 2. Otomatik Yedekleme
```powershell
# Yedekleme scripti oluştur
$backupScript = @"
`$backupDir = "`$env:USERPROFILE\BabaChain_Backups"
`$date = Get-Date -Format "yyyyMMdd_HHmmss"

# Yedekleme dizinini oluştur
New-Item -ItemType Directory -Force -Path `$backupDir

# Cüzdan dosyasını yedekle
if (Test-Path "`$env:APPDATA\BabaChain\wallet.dat") {
    Copy-Item "`$env:APPDATA\BabaChain\wallet.dat" "`$backupDir\wallet_`$date.dat"
}

# Konfigürasyonu yedekle
Copy-Item "`$env:APPDATA\BabaChain\babachain.conf" "`$backupDir\babachain_conf_`$date.conf"

# Eski yedekleri temizle (30 günden eski)
Get-ChildItem "`$backupDir\wallet_*.dat" | Where-Object {`$_.LastWriteTime -lt (Get-Date).AddDays(-30)} | Remove-Item
Get-ChildItem "`$backupDir\babachain_conf_*.conf" | Where-Object {`$_.LastWriteTime -lt (Get-Date).AddDays(-30)} | Remove-Item

Write-Host "Yedekleme tamamlandı: `$date"
"@

$backupScript | Out-File -FilePath "$env:USERPROFILE\backup_babachain.ps1" -Encoding UTF8

# Görev Zamanlayıcısı ile günlük yedekleme
$action = New-ScheduledTaskAction -Execute "PowerShell.exe" -Argument "-File `"$env:USERPROFILE\backup_babachain.ps1`""
$trigger = New-ScheduledTaskTrigger -Daily -At "02:00"
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries
Register-ScheduledTask -TaskName "BabaChain Backup" -Action $action -Trigger $trigger -Settings $settings -Description "Daily BabaChain wallet backup"
```

### 3. Sağlık Kontrolü
```powershell
# Sağlık kontrol scripti oluştur
$healthScript = @"
`$logFile = "`$env:USERPROFILE\BabaChain_Health.log"
`$date = Get-Date -Format "yyyy-MM-dd HH:mm:ss"

# BabaChain servisinin çalışıp çalışmadığını kontrol et
`$service = Get-Service -Name "BabaChain" -ErrorAction SilentlyContinue
if (`$service -eq `$null -or `$service.Status -ne "Running") {
    Add-Content -Path `$logFile -Value "`$date - HATA: BabaChain servisi çalışmıyor!"
    Start-Service BabaChain -ErrorAction SilentlyContinue
    Add-Content -Path `$logFile -Value "`$date - BabaChain servisi yeniden başlatıldı"
} else {
    # RPC bağlantısını test et
    try {
        `$blockCount = & babachain-cli.exe getblockcount 2>`$null
        Add-Content -Path `$logFile -Value "`$date - OK: BabaChain çalışıyor, blok: `$blockCount"
    } catch {
        Add-Content -Path `$logFile -Value "`$date - UYARI: RPC yanıt vermiyor"
    }
}
"@

$healthScript | Out-File -FilePath "$env:USERPROFILE\health_check_babachain.ps1" -Encoding UTF8

# Her 5 dakikada bir sağlık kontrolü
$action = New-ScheduledTaskAction -Execute "PowerShell.exe" -Argument "-File `"$env:USERPROFILE\health_check_babachain.ps1`""
$trigger = New-ScheduledTaskTrigger -Once -At (Get-Date) -RepetitionInterval (New-TimeSpan -Minutes 5) -RepetitionDuration (New-TimeSpan -Days 365)
Register-ScheduledTask -TaskName "BabaChain Health Check" -Action $action -Trigger $trigger -Description "BabaChain health monitoring"
```

## 📊 İzleme ve Monitoring

### 1. Sistem Kaynaklarını İzleme
```powershell
# CPU ve RAM kullanımı
Get-Process -Name "babachaind" | Select-Object CPU, WorkingSet, VirtualMemorySize

# Disk kullanımı
Get-WmiObject -Class Win32_LogicalDisk | Select-Object DeviceID, Size, FreeSpace
```

### 2. Ağ İstatistikleri
```powershell
# Ağ bağlantıları
netstat -an | findstr 9999

# Port kullanımı
Get-NetTCPConnection -LocalPort 9999
```

## 🔄 Güncelleme Prosedürü

### BabaChain Güncellemesi
```powershell
# Mevcut versiyonu kontrol et
babachain-cli.exe getnetworkinfo | findstr version

# Servisi durdur
Stop-Service BabaChain

# Kaynak kodu güncelle
cd C:\BabaChain
git fetch origin
git checkout mainnet-v1.1  # Yeni versiyon

# Yeniden derle
cd build
cmake --build . --config Release --parallel

# Binary'leri güncelle
copy Release\babachaind.exe C:\Windows\System32\
copy Release\babachain-cli.exe C:\Windows\System32\
copy Release\babachain-qt.exe C:\Windows\System32\

# Servisi başlat
Start-Service BabaChain

# Güncellemeyi doğrula
babachain-cli.exe getnetworkinfo | findstr version
```

## 🚨 Sorun Giderme

### Yaygın Sorunlar ve Çözümleri

#### 1. Derleme Hatası
```powershell
# Visual Studio Build Tools'u kontrol et
Get-WmiObject -Class Win32_Product | Where-Object {$_.Name -like "*Visual Studio*"}

# vcpkg paketlerini yeniden yükle
cd C:\vcpkg
.\vcpkg remove --outdated
.\vcpkg install boost:x64-windows openssl:x64-windows
```

#### 2. Servis Başlamıyor
```powershell
# Servis loglarını kontrol et
Get-EventLog -LogName Application -Source "BabaChain" -Newest 10

# NSSM ile servis durumunu kontrol et
nssm status BabaChain

# Manuel başlatma testi
babachaind.exe -printtoconsole
```

#### 3. Staking Çalışmıyor
```powershell
# Staking durumunu detaylı kontrol et
babachain-cli.exe getstakinginfo

# Cüzdan kilidini kontrol et
babachain-cli.exe walletinfo

# Cüzdanı staking için aç
babachain-cli.exe walletpassphrase "ŞİFRE" 999999999 $true
```

## 📈 Performans Optimizasyonu

### 1. Windows Optimizasyonu
```powershell
# Yüksek performans güç planını etkinleştir
powercfg -setactive 8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c

# Disk optimizasyonu
Optimize-Volume -DriveLetter C -ReTrim
```

### 2. Ağ Optimizasyonu
```powershell
# TCP ayarlarını optimize et
netsh int tcp set global autotuninglevel=normal
netsh int tcp set global chimney=enabled
netsh int tcp set global rss=enabled
```

## 🎯 Sonuç

Bu rehberi takip ederek BabaChain blockchain'inizi Windows üzerinde başarıyla çalıştırabilirsiniz. Sistem otomatik olarak:

- ✅ Mainnet'e bağlanacak
- ✅ Blockchain'i senkronize edecek  
- ✅ Staking yapmaya başlayacak
- ✅ Windows servisi olarak çalışacak

**Önemli Notlar:**
- Cüzdan şifrelerinizi güvenli yerde saklayın
- Düzenli yedekleme yapın
- Windows güncellemelerini takip edin
- Antivirus yazılımınızda BabaChain'i beyaz listeye ekleyin

**Destek için:**
- GitHub: https://github.com/Baba-Chain/BabaChain
- Discord: https://discord.gg/babachain
- Telegram: https://t.me/babachainofficial