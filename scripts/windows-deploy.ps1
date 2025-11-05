# BabaChain Windows Otomatik Kurulum Scripti
# Bu script BabaChain'i Windows üzerinde otomatik olarak kurar ve yapılandırır

# Yönetici yetkisi kontrolü
if (-NOT ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Host "Bu script yönetici yetkisi ile çalıştırılmalıdır!" -ForegroundColor Red
    Write-Host "PowerShell'i 'Yönetici olarak çalıştır' seçeneği ile açın." -ForegroundColor Yellow
    Read-Host "Devam etmek için Enter'a basın"
    exit 1
}

# Renkli çıktı fonksiyonları
function Write-Status {
    param($Message)
    Write-Host "[BİLGİ] $Message" -ForegroundColor Blue
}

function Write-Success {
    param($Message)
    Write-Host "[BAŞARILI] $Message" -ForegroundColor Green
}

function Write-Warning {
    param($Message)
    Write-Host "[UYARI] $Message" -ForegroundColor Yellow
}

function Write-Error {
    param($Message)
    Write-Host "[HATA] $Message" -ForegroundColor Red
}

Write-Status "🪟 BabaChain Windows Kurulum Scripti Başlatılıyor..."

# Sistem bilgilerini göster
$osInfo = Get-WmiObject -Class Win32_OperatingSystem
$cpuInfo = Get-WmiObject -Class Win32_Processor
$memInfo = Get-WmiObject -Class Win32_ComputerSystem

Write-Status "Sistem bilgileri:"
Write-Host "İşletim Sistemi: $($osInfo.Caption) $($osInfo.Version)"
Write-Host "Mimari: $($osInfo.OSArchitecture)"
Write-Host "CPU: $($cpuInfo.Name)"
Write-Host "RAM: $([math]::Round($memInfo.TotalPhysicalMemory / 1GB, 2)) GB"

# Windows 10/11 kontrolü
if ($osInfo.Version -lt "10.0") {
    Write-Error "Bu script Windows 10 veya üzeri gerektirir"
    exit 1
}

# Onay al
$confirmation = Read-Host "Kuruluma devam etmek istiyor musunuz? (y/N)"
if ($confirmation -ne 'y' -and $confirmation -ne 'Y') {
    Write-Warning "Kurulum iptal edildi."
    exit 1
}

# 1. Chocolatey kurulumu
Write-Status "🍫 Chocolatey kontrol ediliyor..."
if (!(Get-Command choco -ErrorAction SilentlyContinue)) {
    Write-Status "Chocolatey yükleniyor..."
    Set-ExecutionPolicy Bypass -Scope Process -Force
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
    iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
    Write-Success "Chocolatey yüklendi"
} else {
    Write-Success "Chocolatey zaten yüklü"
}

# 2. Gerekli araçları yükle
Write-Status "🛠️ Gerekli araçlar yükleniyor..."
$packages = @(
    "visualstudio2022buildtools",
    "visualstudio2022-workload-vctools",
    "git",
    "cmake",
    "python",
    "7zip",
    "nssm"
)

foreach ($package in $packages) {
    Write-Status "Yükleniyor: $package"
    choco install $package -y --no-progress
}

Write-Success "Gerekli araçlar yüklendi"

# 3. vcpkg kurulumu
Write-Status "📦 vcpkg kurulumu..."
if (!(Test-Path "C:\vcpkg")) {
    Write-Status "vcpkg klonlanıyor..."
    Set-Location C:\
    git clone https://github.com/Microsoft/vcpkg.git
    Set-Location C:\vcpkg
    .\bootstrap-vcpkg.bat
    Write-Success "vcpkg kuruldu"
} else {
    Write-Success "vcpkg zaten mevcut"
    Set-Location C:\vcpkg
}

# 4. Gerekli paketleri yükle
Write-Status "📚 Bağımlılıklar yükleniyor... (Bu işlem uzun sürebilir)"
$vcpkgPackages = @(
    "boost:x64-windows",
    "openssl:x64-windows",
    "berkeleydb:x64-windows",
    "zeromq:x64-windows",
    "miniupnpc:x64-windows",
    "protobuf:x64-windows",
    "qt5:x64-windows",
    "libevent:x64-windows",
    "qrencode:x64-windows"
)

foreach ($package in $vcpkgPackages) {
    Write-Status "vcpkg paketi yükleniyor: $package"
    .\vcpkg install $package
}

# vcpkg'yi global olarak entegre et
.\vcpkg integrate install
Write-Success "Bağımlılıklar yüklendi"

# 5. BabaChain kaynak kodunu indir
Write-Status "📥 BabaChain kaynak kodu indiriliyor..."
Set-Location C:\
if (!(Test-Path "C:\BabaChain")) {
    git clone https://github.com/BabaChain/BabaChain.git
    Write-Success "Kaynak kod indirildi"
} else {
    Write-Warning "Kaynak kod zaten mevcut, güncelleniyor..."
    Set-Location C:\BabaChain
    git pull origin main
}

# 6. BabaChain'i derle
Write-Status "🔨 BabaChain derleniyor... (Bu işlem uzun sürebilir)"
Set-Location C:\BabaChain

# Build dizinini oluştur
if (!(Test-Path "build")) {
    New-Item -ItemType Directory -Name "build"
}
Set-Location build

# CMake ile yapılandır
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=Release

# Derle
$cpuCores = (Get-WmiObject -Class Win32_ComputerSystem).NumberOfLogicalProcessors
Write-Status "🔧 $cpuCores çekirdek kullanılarak derleniyor..."
cmake --build . --config Release --parallel $cpuCores

# Binary'leri kopyala
Write-Status "📋 Binary'ler kopyalanıyor..."
Copy-Item "Release\babachaind.exe" "C:\Windows\System32\"
Copy-Item "Release\babachain-cli.exe" "C:\Windows\System32\"
if (Test-Path "Release\babachain-qt.exe") {
    Copy-Item "Release\babachain-qt.exe" "C:\Windows\System32\"
}

Write-Success "BabaChain başarıyla derlendi ve yüklendi"

# 7. Konfigürasyon dosyası oluştur
Write-Status "⚙️ Konfigürasyon dosyaları oluşturuluyor..."
$dataDir = "$env:APPDATA\BabaChain"
New-Item -ItemType Directory -Force -Path $dataDir

# RPC şifresi oluştur
$rpcPassword = [System.Web.Security.Membership]::GeneratePassword(32, 8)

$configContent = @"
# BabaChain Windows Konfigürasyonu
# Otomatik olarak oluşturuldu: $(Get-Date)

# RPC ayarları
rpcuser=babachain_user
rpcpassword=$rpcPassword
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

# Windows optimizasyonları
uacomment=BabaChain-Windows-Node
"@

$configContent | Out-File -FilePath "$dataDir\babachain.conf" -Encoding UTF8

Write-Success "Konfigürasyon dosyası oluşturuldu"
Write-Warning "RPC şifresi: $rpcPassword (Bu şifreyi güvenli bir yerde saklayın!)"

# 8. Windows servisi oluştur
Write-Status "🔧 Windows servisi oluşturuluyor..."
nssm install BabaChain "C:\Windows\System32\babachaind.exe"
nssm set BabaChain Parameters "-daemon"
nssm set BabaChain DisplayName "BabaChain Daemon"
nssm set BabaChain Description "BabaChain Proof-of-Stake Cryptocurrency Node"
nssm set BabaChain Start SERVICE_AUTO_START

Write-Success "Windows servisi oluşturuldu"

# 9. Firewall kuralları ekle
Write-Status "🛡️ Firewall kuralları ekleniyor..."
New-NetFirewallRule -DisplayName "BabaChain P2P" -Direction Inbound -Protocol TCP -LocalPort 9999 -Action Allow
New-NetFirewallRule -DisplayName "BabaChain RPC" -Direction Inbound -Protocol TCP -LocalPort 9998 -Action Allow -RemoteAddress LocalSubnet

Write-Success "Firewall kuralları eklendi"

# 10. Yedekleme scripti oluştur
Write-Status "💾 Yedekleme scripti oluşturuluyor..."
$backupScript = @"
`$backupDir = "`$env:USERPROFILE\BabaChain_Backups"
`$date = Get-Date -Format "yyyyMMdd_HHmmss"

New-Item -ItemType Directory -Force -Path `$backupDir

if (Test-Path "`$env:APPDATA\BabaChain\wallet.dat") {
    Copy-Item "`$env:APPDATA\BabaChain\wallet.dat" "`$backupDir\wallet_`$date.dat"
}

Copy-Item "`$env:APPDATA\BabaChain\babachain.conf" "`$backupDir\babachain_conf_`$date.conf"

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

Write-Success "Yedekleme scripti oluşturuldu ve zamanlandı"

# 11. Sağlık kontrol scripti oluştur
Write-Status "🏥 Sağlık kontrol scripti oluşturuluyor..."
$healthScript = @"
`$logFile = "`$env:USERPROFILE\BabaChain_Health.log"
`$date = Get-Date -Format "yyyy-MM-dd HH:mm:ss"

`$service = Get-Service -Name "BabaChain" -ErrorAction SilentlyContinue
if (`$service -eq `$null -or `$service.Status -ne "Running") {
    Add-Content -Path `$logFile -Value "`$date - HATA: BabaChain servisi çalışmıyor!"
    Start-Service BabaChain -ErrorAction SilentlyContinue
    Add-Content -Path `$logFile -Value "`$date - BabaChain servisi yeniden başlatıldı"
} else {
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

Write-Success "Sağlık kontrol scripti oluşturuldu"

# 12. PowerShell profili güncelle
Write-Status "🔧 PowerShell aliasları oluşturuluyor..."
$profilePath = $PROFILE.CurrentUserAllHosts
if (!(Test-Path $profilePath)) {
    New-Item -ItemType File -Path $profilePath -Force
}

$aliases = @"

# BabaChain aliasları
function baba-cli { babachain-cli.exe `$args }
function baba-qt { babachain-qt.exe `$args }
function baba-status { babachain-cli.exe getblockchaininfo }
function baba-balance { babachain-cli.exe getbalance }
function baba-staking { babachain-cli.exe getstakinginfo }
function baba-peers { babachain-cli.exe getpeerinfo | Select-String "addr" }
function baba-logs { Get-Content "`$env:APPDATA\BabaChain\debug.log" -Tail 50 -Wait }
function baba-start { Start-Service BabaChain }
function baba-stop { Stop-Service BabaChain }
function baba-restart { Restart-Service BabaChain }
function baba-service { Get-Service BabaChain }
"@

Add-Content -Path $profilePath -Value $aliases

Write-Success "PowerShell aliasları eklendi"

# 13. BabaChain servisini başlat
Write-Status "🚀 BabaChain servisi başlatılıyor..."
Start-Service BabaChain

# Başlatma kontrolü
Start-Sleep -Seconds 5
$service = Get-Service -Name "BabaChain" -ErrorAction SilentlyContinue
if ($service -and $service.Status -eq "Running") {
    Write-Success "BabaChain servisi başarıyla başlatıldı!"
} else {
    Write-Error "BabaChain servisi başlatılamadı. Event Viewer'ı kontrol edin."
    exit 1
}

# 14. Kurulum özeti
Write-Success "🎉 BabaChain kurulumu tamamlandı!"
Write-Host ""
Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "                    KURULUM ÖZETİ" -ForegroundColor Cyan
Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host ""
Write-Host "📍 Kurulum Dizini: C:\BabaChain"
Write-Host "📁 Veri Dizini: $env:APPDATA\BabaChain"
Write-Host "⚙️ Konfigürasyon: $env:APPDATA\BabaChain\babachain.conf"
Write-Host "🔑 RPC Şifresi: $rpcPassword"
Write-Host ""
Write-Host "🔧 KULLANIŞLI KOMUTLAR:"
Write-Host "  Servis durumu:      Get-Service BabaChain"
Write-Host "  Event logları:      Get-EventLog -LogName Application -Source BabaChain"
Write-Host "  Blockchain bilgisi: babachain-cli getblockchaininfo"
Write-Host "  Staking durumu:     babachain-cli getstakinginfo"
Write-Host "  Bakiye kontrolü:    babachain-cli getbalance"
Write-Host "  GUI Wallet:         babachain-qt"
Write-Host ""
Write-Host "📚 SONRAKİ ADIMLAR:"
Write-Host "  1. Blockchain senkronizasyonunu bekleyin (birkaç saat sürebilir)"
Write-Host "  2. Cüzdan oluşturun: babachain-cli createwallet `"ana_cuzdan`""
Write-Host "  3. Adres oluşturun: babachain-cli getnewaddress"
Write-Host "  4. BabaChain token'larınızı bu adrese gönderin"
Write-Host "  5. Staking'e başlayın!"
Write-Host ""
Write-Host "🔗 DESTEK:"
Write-Host "  GitHub: https://github.com/BabaChain/BabaChain"
Write-Host "  Discord: https://discord.gg/babachain"
Write-Host "  Telegram: https://t.me/babachainofficial"
Write-Host ""
Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan

Write-Warning "PowerShell'i yeniden başlatarak yeni aliasları aktif edin"
Write-Success "Kurulum başarıyla tamamlandı! 🎉"

Read-Host "Devam etmek için Enter'a basın"