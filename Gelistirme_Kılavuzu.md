# BabaChain Geliştirme Kılavuzu

## Genel Bakış

BabaChain, bir kripto para birimi olan BabaChain'in referans uygulamasıdır. Öncelikle C++20 ile yazılmış (en az Clang 16 veya GCC 11.1 gerektirir) Bitcoin Core kod tabanı üzerine inşa edilmiştir. BabaChain, GNU Autotools derleme sistemini kullanır.

## Dizin Yapısı

- **Uygulama** `src/` - C++20 kod tabanı
  - `src/bench/` - Performans karşılaştırmaları (`nanobench` kullanır)
  - `src/fuzz/` - Fuzzing test araçları
  - `src/index/` - İsteğe bağlı indeksler
  - `src/interfaces/` - Kod tabanı izolasyonu ve süreçler arası iletişim arayüzleri
  - `src/qt/` - BabaChain Qt GUI uygulaması (Qt 5 kullanır)
  - `src/rpc/` - JSON-RPC sunucusu ve uç noktaları
  - `src/util/` - Yardımcı fonksiyonlar
  - `src/wallet/` - Cüzdan uygulaması (Berkeley DB ve SQLite kullanır)
  - `src/zmq/` - Gerçek zamanlı olay yayını için ZeroMQ bildirim desteği
- **Birim Testleri**
  - `src/test/`, `src/wallet/test/` - C++20 birim testleri (`Boost::Test` kullanır)
  - `src/qt/test/` - GUI uygulaması için C++20 birim testleri (Qt 5 kullanır)
- **Fonksiyonel Testler**: `test/functional/` - `babachaind` ve `babachain-node`'a bağımlı Python testleri (minimum sürüm `.python-version` dosyasında)

### Hariç Tutulacak Dizinler

**Hiçbir koşulda** aşağıdakilerde değişiklik yapmayın:
- `guix-build*` - Derleme sistemi dosyaları
- `releases` - Sürüm yapıları
- Dahili bağımlılıklar:
  - `src/{crc32c,babachainbls,gsl,immer,leveldb,minisketch,secp256k1,univalue}`
  - `src/crypto/{ctaes,x11}`

**Özel olarak istenmediği sürece** kaçının:
- `.github` - GitHub iş akışları ve yapılandırmaları
- `depends` - Bağımlılık derleme sistemi
- `ci` - Sürekli entegrasyon
- `contrib` - Katkı sağlanan betikler
- `doc` - Dokümantasyon

## Derleme Komutları

### Kurulum ve Derleme
```bash
# Derleme sistemini oluştur
./autogen.sh

# Mevcut platform için bağımlılıkları derle
make -C depends -j"$(( $(nproc) - 1 ))" | tail 5

# Bağımlılıklarla yapılandır (depends derleme çıktısında gösterilen yolu kullan)
# Örnek yollar: depends/x86_64-pc-linux-gnu, depends/aarch64-apple-darwin24.3.0
./configure --prefix=$(pwd)/depends/[platform-triplet]

# Geliştirici yapılandırması (önerilen)
./configure --prefix=$(pwd)/depends/[platform-triplet] \
            --disable-hardening \
            --enable-crash-hooks \
            --enable-debug \
            --enable-reduce-exports \
            --enable-stacktraces \
            --enable-suppress-external-warnings \
            --enable-werror

# Paralel işlerle derle (bir çekirdek boş bırakarak)
make -j"$(( $(nproc) - 1 ))"
```

## Test Komutları

### Birim Testleri
```bash
# Tüm birim testlerini çalıştır
make check

# Belirli bir testi çalıştır (örn. getarg_tests)
./src/test/test_babachain --run_test=getarg_tests

# Birim testlerini debug et
gdb ./src/test/test_babachain
```

### Fonksiyonel Testler
```bash
# Tüm fonksiyonel testleri çalıştır
test/functional/test_runner.py

# Belirli bir testi çalıştır
test/functional/wallet_hd.py

# Genişletilmiş test paketi
test/functional/test_runner.py --extended

# Paralel çalıştırma
test/functional/test_runner.py -j$(nproc)

# Debug seçenekleri
test/functional/test_runner.py --nocleanup --tracerpc -l DEBUG
```

### Kod Kalitesi
```bash
# Tüm linting'i çalıştır
test/lint/all-lint.py

# Yaygın bireysel kontroller
test/lint/lint-python.py
test/lint/lint-shell.py
test/lint/lint-whitespace.py
test/lint/lint-circular-dependencies.py
```

## Üst Düzey Mimari

BabaChain , katmanlı bir mimari kullanarak Bitcoin Core'u kompozisyon yoluyla genişletir:

```
BabaChain Bileşenleri
├── Bitcoin Core Temeli (Blockchain, konsensüs, ağ)
├── Ana Düğümler (Altyapı)
│   ├── LLMQ (Kuorum altyapısı)
│   │   ├── InstantSend (İşlem kilitleme)
│   │   ├── ChainLocks (Blok kesinliği)
│   │   ├── EHF Sinyalleri (Hard fork koordinasyonu)
│   │   └── Platform/Evolution (Kredi Havuzu, Varlık Kilitleri)
│   ├── CoinJoin (Coin karıştırma)
│   └── Yönetişim Oylaması (Ana düğümler teklifleri oylar)
├── Yönetişim Sistemi (Teklif gönderimi/yönetimi)
└── Spork Sistemi (Özellik kontrolü)
```

### Temel Mimari Bileşenler

#### Ana Düğümler (`src/masternode/`, `src/evo/`)
- **Deterministik Ana Düğüm Listeleri**: Değişmez veri yapıları kullanan konsensüs-kritik kayıt
- **Aktif Ana Düğüm Yöneticisi**: Yerel ana düğüm işlemleri ve BLS anahtar yönetimi
- **Özel İşlemler**: Ana düğüm yaşam döngüsü için ProRegTx, ProUpServTx, ProUpRegTx, ProUpRevTx

#### Uzun Yaşayan Ana Düğüm Kurumları (`src/llmq/`)
- **Kuorum Türleri**: Farklı hizmetler için çoklu yapılandırmalar (50/60, 400/60, 400/85)
- **Dağıtık Anahtar Üretimi**: Kriptografik olarak güvenli kuorum oluşumu
- **Hizmetler**: ChainLocks (%51 saldırı önleme), InstantSend, yönetişim oylaması

#### CoinJoin Gizliliği (`src/coinjoin/`)
- **Karıştırma Mimarisi**: Ana düğüm koordineli karıştırma oturumları
- **Denominasyon**: Gizlilik için tekdüzen çıktılar
- **Oturum Yönetimi**: Çok taraflı işlem oluşturma

#### Yönetişim (`src/governance/`)
- **Yönetişim Nesneleri**: Teklifler, tetikleyiciler, süperblok yönetimi
- **Hazine**: Yönetişim oylarına dayalı otomatik ödemeler
- **Oylama**: Zincir üstü teklif oylaması ve sayımı

#### Evolution Veritabanı (`src/evo/evodb`)
- **Özelleşmiş Depolama**: Ana düğüm anlık görüntüleri, kuorum durumu, yönetişim nesneleri
- **Verimli Güncellemeler**: Ana düğüm listeleri için diferansiyel güncellemeler
- **Kredi Havuzu Yönetimi**: Platform entegrasyon desteği

#### BabaChain'e Özgü Veritabanları

- **CFlatDB**: Kalıcı depolama için kullanılan BabaChain'e özgü düz dosya veritabanı formatı
  - `MasternodeMetaStore`: Ana düğüm metadata kalıcılığı
  - `GovernanceStore`: Yönetişim nesnesi depolaması
  - `SporkStore`: Spork durum kalıcılığı
  - `NetFulfilledRequestStore`: Ağ isteği takibi
- **CDBWrapper**: BabaChain'e özgü veriler için genişletilmiş Bitcoin Core veritabanı sarmalayıcısı
  - `CDKGSessionManager`: LLMQ DKG oturum kalıcılığı
  - `CEvoDb`: Evolution/deterministik ana düğüm verileri için özelleşmiş veritabanı
  - `CInstantSendDb`: InstantSend kilit kalıcılığı
  - `CQuorumManager`: Kuorum durum depolaması
  - `CRecoveredSigsDb`: LLMQ kurtarılan imza depolaması

### Entegrasyon Kalıpları

#### Başlatma Akışı
1. **Temel Kurulum**: Çekirdek Bitcoin başlatması
2. **Parametre Etkileşimi**: BabaChain'e özgü yapılandırma doğrulaması
3. **Arayüz Kurulumu**: NodeContext'te BabaChain yöneticisi örneklendirmesi
4. **Ana Başlatma**: EvoDb, ana düğüm sistemi, LLMQ, yönetişim başlatması

#### Konsensüs Entegrasyonu
- **Blok Doğrulama Uzantıları**: Özel işlem doğrulaması
- **Mempool Uzantıları**: Gelişmiş işlem aktarımı
- **Zincir Durum Uzantıları**: Ana düğüm listesi ve kuorum durum takibi
- **Fork Önleme**: ChainLocks yeniden düzenlemeleri önler

#### Temel Tasarım Kalıpları
- **Yönetici Kalıbı**: Her alt sistem için merkezi yöneticiler
- **Olay Güdümlü Mimari**: ValidationInterface geri çağrıları alt sistemleri koordine eder
- **Değişmez Veri Yapıları**: Immer kütüphanesi kullanarak verimli ana düğüm listesi yönetimi
- **Değişiklik Yerine Uzantı**: Bitcoin Core temeline minimal değişiklikler

### Kritik Arayüzler
- **NodeContext**: Merkezi bağımlılık enjeksiyon konteyneri
- **LLMQContext**: LLMQ'ya özgü bağlam ve durum yönetimi
- **ValidationInterface**: Blok/işlem işleme için olay dağıtımı
- **ChainstateManager**: BabaChain'e özgü doğrulama ile geliştirilmiş
- **Chainstate Başlatma**: `src/node/chainstate.*` içine ayrılmış
- **Özel İşlem Serileştirme**: Yük serileştirme rutinleri (`src/evo/specialtx.h`)
- **BLS Entegrasyonu**: Gelişmiş özellikler için kriptografik temel

## Geliştirme İş Akışı

### Yaygın Görevler
```bash
# Temiz derleme
make clean

# Debug loglama ile babachaind çalıştır
./src/babachaind -debug=all -printtoconsole

# Özel babachaind ile fonksiyonel test çalıştır
test/functional/test_runner.py --babachaind=/path/to/babachaind

# IDE'ler için compile_commands.json oluştur
bear -- make -j"$(( $(nproc) - 1 ))"
```

### Hata Ayıklama
```bash
# babachaind'i debug et
gdb ./src/babachaind

# Performans profilleme
test/functional/test_runner.py --perf
perf report -i /path/to/datadir/test.perf.data --stdio | c++filt

# Bellek hata ayıklama
valgrind --leak-check=full ./src/babachaind
```

### `gh` CLI ile GitHub CI Hata Ayıklama

```bash
# JSON çıktısı ile detaylı kontrol bilgisi al
gh pr checks <PR_NUMBER> --json name,state,link,description

# Başarısız veya bekleyen kontroller için filtrele
gh pr checks <PR_NUMBER> --json name,state,link --jq '.[] | select(.state == "FAILURE" or .state == "PENDING")'

# Belirli bir CI işinden logları görüntüle
gh api repos/baba-chain/babachain/actions/jobs/<JOB_ID>/logs

# Bir çalıştırmadan başarısız işleri ve adımları filtrele
gh run view <RUN_ID> --json jobs --jq '.jobs[] | select(.conclusion == "failure") | {name, conclusion}'

# Örnek: PR 6691 için lint başarısızlık loglarını al
# gh api repos/baba-chain/babachain/actions/jobs/46274126203/logs
```

## Dal Yapısı

- `master`: Kararlı sürümler
- `develop`: Aktif geliştirme (düzenli olarak derlenir ve test edilir)

## Önemli Notlar

- Paralel derlemeler için `make -j"$(( $(nproc) - 1 ))"` kullanın (bir çekirdeği boş bırakır)
- Commit'lerden önce her zaman linting çalıştırın: `test/lint/all-lint.py`
- Bellek kısıtlı sistemler için configure sırasında özel CXXFLAGS kullanın
- Özel işlemler yük uzantıları kullanır - `src/evo/specialtx.h`'ye bakın
- Ana düğüm listeleri thread güvenliği için değişmez veri yapıları (Immer kütüphanesi) kullanır
- LLMQ kurumları farklı amaçlar için farklı yapılandırmalara sahiptir
- BabaChain, LRU çıkarma ile verimli önbellekleme için `unordered_lru_cache` kullanır
- Kod tabanı performans için BabaChain'e özgü veri yapılarını yaygın olarak kullanır

## Türkçe Dil Desteği

BabaChain Core, tüm platformlarda kapsamlı Türkçe dil desteği içerir:
- Masaüstü Qt Cüzdanı: Tam Türkçe çeviri ve yerel ayar desteği
- Android Mobil Cüzdan: Türkçe string kaynakları ve UTF-8 kodlama
- iOS Mobil Cüzdan: Türkçe yerelleştirme ve erişilebilirlik desteği
- RPC Arayüzü: Türkçe karakter doğrulaması ve UTF-8 kodlama
- Veritabanı: Türkçe karakterler için UTF-8 desteği
- Loglama Sistemi: Türkçe karakter işleme

Detaylı bilgi için `doc/turkish-language-support.md` dosyasına bakın.
