FROM ubuntu:20.04 as builder
RUN apt-get update && \
	DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential cmake clang libssl-dev zlib1g-dev gperf wget git ninja-build libsodium-dev libmicrohttpd-dev pkg-config autoconf automake libtool && \
	rm -rf /var/lib/apt/lists/*
ENV CC clang
ENV CXX clang++
ENV CCACHE_DISABLE 1

WORKDIR /
RUN git clone https://github.com/libbitcoin/secp256k1.git
WORKDIR /secp256k1
RUN  ./autogen.sh && \
     ./configure --enable-module-recovery && \
     make

WORKDIR /
RUN git clone https://github.com/openssl/openssl openssl_3
WORKDIR /openssl_3
RUN git checkout openssl-3.1.4 && \
    ./config && \
    make build_libs -j4

WORKDIR /

RUN git clone --recursive https://github.com/ton-blockchain/ton
WORKDIR /ton

RUN mkdir build && \
	cd build && \
	cmake -GNinja -DOPENSSL_FOUND=1 -DOPENSSL_INCLUDE_DIR=/openssl_3/include -DOPENSSL_CRYPTO_LIBRARY=/openssl_3/libcrypto.a -DCMAKE_BUILD_TYPE=Release -DSECP256K1_FOUND=1 -DSECP256K1_INCLUDE_DIR=/secp256k1/include -DSECP256K1_LIBRARY=/secp256k1/.libs/libsecp256k1.a -DPORTABLE=1 -DTON_ARCH= .. && \
	ninja storage-daemon storage-daemon-cli tonlibjson fift func validator-engine validator-engine-console generate-random-id dht-server lite-client

FROM ubuntu:20.04
RUN apt-get update && \
    apt-get install -y wget libatomic1 && \
    rm -rf /var/lib/apt/lists/*

RUN mkdir -p /var/ton-work/db && \
    mkdir -p /var/ton-work/db/static

COPY --from=builder /ton/build/storage/storage-daemon/storage-daemon /usr/local/bin/
COPY --from=builder /ton/build/storage/storage-daemon/storage-daemon-cli /usr/local/bin/
COPY --from=builder /ton/build/lite-client/lite-client /usr/local/bin/
COPY --from=builder /ton/build/validator-engine/validator-engine /usr/local/bin/
COPY --from=builder /ton/build/validator-engine-console/validator-engine-console /usr/local/bin/
COPY --from=builder /ton/build/utils/generate-random-id /usr/local/bin/

WORKDIR /var/ton-work/db
COPY init.sh control.template ./
RUN chmod +x init.sh

ENTRYPOINT ["/var/ton-work/db/init.sh"]
