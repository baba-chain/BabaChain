# BabaChain Docker Image
FROM ubuntu:22.04

# Metadata
LABEL maintainer="BabaChain Team <team@babachain.org>"
LABEL description="BabaChain Proof-of-Stake Cryptocurrency Node"
LABEL version="1.0.0"

# Çevre değişkenleri
ENV DEBIAN_FRONTEND=noninteractive
ENV BABACHAIN_USER=babachain
ENV BABACHAIN_HOME=/home/babachain
ENV BABACHAIN_DATA=/home/babachain/.babachain

# Sistem güncellemesi ve gerekli paketlerin yüklenmesi
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    libboost-all-dev \
    libssl-dev \
    libdb++-dev \
    libminiupnpc-dev \
    libzmq3-dev \
    libqt5gui5 \
    libqt5core5a \
    libqt5dbus5 \
    qttools5-dev \
    qttools5-dev-tools \
    libprotobuf-dev \
    protobuf-compiler \
    pkg-config \
    libevent-dev \
    libqrencode-dev \
    gosu \
    && rm -rf /var/lib/apt/lists/*

# BabaChain kullanıcısı oluşturma
RUN useradd -m -s /bin/bash $BABACHAIN_USER && \
    mkdir -p $BABACHAIN_DATA && \
    chown -R $BABACHAIN_USER:$BABACHAIN_USER $BABACHAIN_HOME

# BabaChain kaynak kodunu kopyalama
COPY . $BABACHAIN_HOME/BabaChain/
RUN chown -R $BABACHAIN_USER:$BABACHAIN_USER $BABACHAIN_HOME/BabaChain

# BabaChain'i derleme
WORKDIR $BABACHAIN_HOME/BabaChain
RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc) && \
    make install

# Konfigürasyon dosyası şablonu
COPY docker/babachain.conf.template $BABACHAIN_DATA/babachain.conf.template
RUN chown $BABACHAIN_USER:$BABACHAIN_USER $BABACHAIN_DATA/babachain.conf.template

# Entrypoint scripti
COPY docker/docker-entrypoint.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/docker-entrypoint.sh

# Portları aç
EXPOSE 9999 9998

# Volume'lar
VOLUME ["$BABACHAIN_DATA"]

# Çalışma dizini
WORKDIR $BABACHAIN_HOME

# Entrypoint
ENTRYPOINT ["docker-entrypoint.sh"]
CMD ["babachaind"]