#!/bin/bash
set -e

# BabaChain Docker Entrypoint Script

BABACHAIN_USER=${BABACHAIN_USER:-babachain}
BABACHAIN_DATA=${BABACHAIN_DATA:-/home/babachain/.babachain}
BABACHAIN_CONF=${BABACHAIN_DATA}/babachain.conf

# Konfigürasyon dosyası oluştur (eğer yoksa)
if [ ! -f "$BABACHAIN_CONF" ]; then
    echo "Konfigürasyon dosyası oluşturuluyor..."
    
    # RPC şifresi oluştur
    RPC_PASSWORD=${RPC_PASSWORD:-$(openssl rand -base64 32)}
    
    # Konfigürasyon dosyasını oluştur
    cat > "$BABACHAIN_CONF" << EOF
# BabaChain Docker Konfigürasyonu
# Otomatik oluşturuldu: $(date)

# RPC ayarları
rpcuser=${RPC_USER:-babachain_user}
rpcpassword=${RPC_PASSWORD}
rpcport=${RPC_PORT:-9998}
rpcbind=0.0.0.0
rpcallowip=0.0.0.0/0

# Ağ ayarları
port=${P2P_PORT:-9999}
listen=1
discover=1
upnp=${UPNP:-1}

# PoS staking ayarları
staking=${STAKING:-1}
stakegen=${STAKEGEN:-1}
reservebalance=${RESERVE_BALANCE:-0}

# Seed node'lar
addnode=seed1.babachain.org:9999
addnode=seed2.babachain.org:9999
addnode=seed3.babachain.org:9999
addnode=node1.babachain.network:9999
addnode=node2.babachain.network:9999

# Performans ayarları
maxconnections=${MAX_CONNECTIONS:-125}
timeout=${TIMEOUT:-5000}
dbcache=${DB_CACHE:-300}
maxmempool=${MAX_MEMPOOL:-300}

# Güvenlik
rpcssl=0

# Loglama
debug=${DEBUG:-pos,staking,net}

# Network
testnet=${TESTNET:-0}
regtest=${REGTEST:-0}

# Docker optimizasyonları
printtoconsole=1
EOF

    echo "RPC Şifresi: $RPC_PASSWORD"
fi

# Dosya sahipliğini ayarla
chown -R $BABACHAIN_USER:$BABACHAIN_USER $BABACHAIN_DATA

# Komut argümanlarını işle
if [ "$1" = 'babachaind' ]; then
    echo "BabaChain daemon başlatılıyor..."
    exec gosu $BABACHAIN_USER babachaind -conf=$BABACHAIN_CONF -datadir=$BABACHAIN_DATA -printtoconsole
elif [ "$1" = 'babachain-cli' ]; then
    shift
    exec gosu $BABACHAIN_USER babachain-cli -conf=$BABACHAIN_CONF -datadir=$BABACHAIN_DATA "$@"
else
    exec gosu $BABACHAIN_USER "$@"
fi