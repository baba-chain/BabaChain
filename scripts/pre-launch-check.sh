#!/bin/bash

# BabaChain Pre-Launch Checklist Script
# Bu script lansman öncesi tüm gereksinimleri kontrol eder

set -e

# Renkler
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

# Fonksiyonlar
print_status() {
    echo -e "${BLUE}[BİLGİ]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[✅ BAŞARILI]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[⚠️ UYARI]${NC} $1"
}

print_error() {
    echo -e "${RED}[❌ HATA]${NC} $1"
}

print_header() {
    echo -e "${PURPLE}$1${NC}"
}

# Kontrol sonuçları
TOTAL_CHECKS=0
PASSED_CHECKS=0
FAILED_CHECKS=0
WARNING_CHECKS=0

# Kontrol fonksiyonu
check_item() {
    local description="$1"
    local command="$2"
    local required="$3"  # true/false
    
    ((TOTAL_CHECKS++))
    print_status "Kontrol ediliyor: $description"
    
    if eval "$command" &>/dev/null; then
        print_success "$description"
        ((PASSED_CHECKS++))
        return 0
    else
        if [ "$required" = "true" ]; then
            print_error "$description"
            ((FAILED_CHECKS++))
            return 1
        else
            print_warning "$description (opsiyonel)"
            ((WARNING_CHECKS++))
            return 2
        fi
    fi
}

print_header "🔍 BabaChain Pre-Launch Checklist"
print_status "Lansman öncesi tüm gereksinimler kontrol ediliyor..."
echo

# 1. Sistem Gereksinimleri
print_header "💻 Sistem Gereksinimleri"

check_item "İşletim sistemi uyumluluğu" "uname -s | grep -E 'Linux|Darwin'" true

# RAM kontrolü
check_item "Yeterli RAM (minimum 4GB)" "[ \$(free -m 2>/dev/null | awk '/^Mem:/{print \$2}' || echo 8192) -ge 4096 ]" true

# Disk alanı kontrolü  
check_item "Yeterli disk alanı (minimum 50GB)" "[ \$(df / | awk 'NR==2 {print \$4}') -ge 52428800 ]" true

# CPU çekirdek sayısı
check_item "Yeterli CPU çekirdeği (minimum 2)" "[ \$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2) -ge 2 ]" true

echo

# 2. Gerekli Yazılımlar
print_header "🛠️ Gerekli Yazılımlar"

check_item "Git kurulu" "command -v git" true
check_item "CMake kurulu" "command -v cmake" true
check_item "Make kurulu" "command -v make" true
check_item "GCC/Clang derleyici" "command -v gcc || command -v clang" true
check_item "Python3 kurulu" "command -v python3" true
check_item "OpenSSL kurulu" "command -v openssl" true

echo

# 3. BabaChain Binary'leri
print_header "🔧 BabaChain Binary'leri"

check_item "babachaind binary'si mevcut" "command -v babachaind" true
check_item "babachain-cli binary'si mevcut" "command -v babachain-cli" true
check_item "babachain-qt binary'si mevcut" "command -v babachain-qt" false

# Binary versiyonları
if command -v babachaind &>/dev/null; then
    VERSION=$(babachaind --version 2>/dev/null | head -n1 || echo "Versiyon alınamadı")
    print_status "BabaChain versiyonu: $VERSION"
fi

echo

# 4. Konfigürasyon Dosyaları
print_header "⚙️ Konfigürasyon Dosyaları"

# Ana konfigürasyon dizini
check_item "BabaChain veri dizini mevcut" "[ -d ~/.babachain ]" true

# Konfigürasyon dosyası
check_item "babachain.conf dosyası mevcut" "[ -f ~/.babachain/babachain.conf ]" true

# Konfigürasyon içeriği kontrolü
if [ -f ~/.babachain/babachain.conf ]; then
    check_item "RPC kullanıcısı tanımlı" "grep -q '^rpcuser=' ~/.babachain/babachain.conf" true
    check_item "RPC şifresi tanımlı" "grep -q '^rpcpassword=' ~/.babachain/babachain.conf" true
    check_item "Staking etkin" "grep -q '^staking=1' ~/.babachain/babachain.conf" true
    check_item "Seed node'lar tanımlı" "grep -q 'addnode=' ~/.babachain/babachain.conf" true
fi

echo

# 5. Ağ Bağlantısı
print_header "🌐 Ağ Bağlantısı"

check_item "İnternet bağlantısı" "ping -c 1 8.8.8.8" true
check_item "DNS çözümleme" "nslookup google.com" true

# Seed node'ları kontrol et
SEED_NODES=("seed1.babachain.org" "seed2.babachain.org" "seed3.babachain.org")
for seed in "${SEED_NODES[@]}"; do
    check_item "$seed erişilebilir" "nc -z -w5 $seed 9999" false
done

echo

# 6. Port Kullanımı
print_header "🔌 Port Kullanımı"

check_item "Port 9999 müsait (P2P)" "! netstat -tuln 2>/dev/null | grep -q ':9999 '" true
check_item "Port 9998 müsait (RPC)" "! netstat -tuln 2>/dev/null | grep -q ':9998 '" true

echo

# 7. Güvenlik Ayarları
print_header "🛡️ Güvenlik Ayarları"

# Firewall kontrolü
if command -v ufw &>/dev/null; then
    check_item "UFW firewall kurulu" "command -v ufw" false
    check_item "UFW aktif" "ufw status | grep -q 'Status: active'" false
elif command -v firewall-cmd &>/dev/null; then
    check_item "Firewalld kurulu" "command -v firewall-cmd" false
    check_item "Firewalld aktif" "systemctl is-active firewalld" false
fi

# SSH güvenliği
check_item "SSH servisi güvenli" "[ ! -f /etc/ssh/sshd_config ] || grep -q 'PermitRootLogin no' /etc/ssh/sshd_config" false

echo

# 8. Sistem Servisleri
print_header "🔄 Sistem Servisleri"

# Systemd kontrolü (Linux)
if command -v systemctl &>/dev/null; then
    check_item "Systemd mevcut" "command -v systemctl" false
    
    # BabaChain servisi
    if [ -f /etc/systemd/system/babachaind.service ]; then
        check_item "BabaChain systemd servisi tanımlı" "[ -f /etc/systemd/system/babachaind.service ]" false
        check_item "BabaChain servisi etkin" "systemctl is-enabled babachaind" false
    fi
fi

echo

# 9. Yedekleme Sistemi
print_header "💾 Yedekleme Sistemi"

check_item "Yedekleme scripti mevcut" "[ -f ~/backup_babachain.sh ]" false
check_item "Yedekleme dizini mevcut" "[ -d ~/BabaChain_Backups ] || [ -d ~/backups ]" false

# Crontab kontrolü
check_item "Crontab yedekleme görevi" "crontab -l 2>/dev/null | grep -q backup" false

echo

# 10. Monitoring ve Loglar
print_header "📊 Monitoring ve Loglar"

check_item "Log dizini mevcut" "[ -d ~/.babachain ] && [ -w ~/.babachain ]" true
check_item "Sağlık kontrol scripti" "[ -f ~/health_check_babachain.sh ] || [ -f ~/health_check.sh ]" false

echo

# 11. Zaman Senkronizasyonu
print_header "⏰ Zaman Senkronizasyonu"

check_item "Sistem zamanı doğru" "[ \$(date +%s) -gt 1700000000 ]" true

# NTP kontrolü
if command -v timedatectl &>/dev/null; then
    check_item "NTP senkronizasyonu aktif" "timedatectl status | grep -q 'NTP synchronized: yes'" false
elif command -v ntpq &>/dev/null; then
    check_item "NTP servisi çalışıyor" "ntpq -p" false
fi

echo

# 12. Genesis Block Parametreleri
print_header "🎯 Genesis Block Parametreleri"

# Chainparams dosyasını kontrol et
if [ -f src/chainparams.cpp ]; then
    check_item "Mainnet genesis timestamp doğru" "grep -q '1764547200' src/chainparams.cpp" true
    check_item "Premine amount doğru" "grep -q '20000000 \* COIN' src/chainparams.cpp" true
    check_item "Network magic bytes doğru" "grep -q '0xbaba1337' src/chainparams.cpp" true
else
    print_warning "chainparams.cpp dosyası bulunamadı (kaynak kod dizininde değilsiniz)"
fi

echo

# Sonuçları özetle
print_header "📋 Kontrol Özeti"
echo "═══════════════════════════════════════════════════════════════"
echo "Toplam Kontrol: $TOTAL_CHECKS"
echo "✅ Başarılı: $PASSED_CHECKS"
echo "❌ Başarısız: $FAILED_CHECKS"
echo "⚠️ Uyarı: $WARNING_CHECKS"
echo "═══════════════════════════════════════════════════════════════"

# Başarı oranı hesapla
SUCCESS_RATE=$(( (PASSED_CHECKS * 100) / TOTAL_CHECKS ))
echo "Başarı Oranı: %$SUCCESS_RATE"

# Sonuç değerlendirmesi
if [ $FAILED_CHECKS -eq 0 ]; then
    if [ $WARNING_CHECKS -eq 0 ]; then
        print_success "🎉 Tüm kontroller başarılı! Lansmana hazırsınız."
        exit 0
    else
        print_warning "⚠️ Bazı opsiyonel özellikler eksik, ancak lansmana hazırsınız."
        exit 0
    fi
else
    print_error "❌ $FAILED_CHECKS kritik sorun var. Lansmanı ertelemelisiniz."
    echo
    print_status "Sorunları çözmek için:"
    echo "1. Başarısız kontrolleri tekrar gözden geçirin"
    echo "2. Gerekli yazılımları yükleyin"
    echo "3. Konfigürasyon dosyalarını düzeltin"
    echo "4. Bu scripti tekrar çalıştırın"
    exit 1
fi