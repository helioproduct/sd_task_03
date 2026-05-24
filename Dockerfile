FROM ghcr.io/userver-framework/ubuntu-22.04-userver-pg-dev:latest

RUN apt-get update && \
    apt-get install -y \
    libsqlite3-dev \
    sqlite3 \
    wget \
    libssl-dev \
    git \
    nlohmann-json3-dev \
    python3 \
    python3-pip && \
    rm -rf /var/lib/apt/lists/* && \
    pip3 install --no-cache-dir pytest requests && \
    cd /tmp && \
    git clone --depth 1 --branch v0.7.0 https://github.com/Thalhammer/jwt-cpp.git && \
    cd jwt-cpp && \
    mkdir build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DJWT_BUILD_EXAMPLES=OFF \
          -DJWT_BUILD_TESTS=OFF \
          -DJWT_INSTALL=ON \
          -DCMAKE_INSTALL_PREFIX=/usr/local \
          -DDEFAULT_NLOHMANN_JSON_BACKEND=OFF \
          .. && \
    make install && \
    ldconfig && \
    rm -rf /tmp/jwt-cpp

WORKDIR /app

COPY . /app

RUN mkdir -p /app/build && \
    cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DUSERVER_FEATURE_POSTGRESQL=OFF \
          -DUSERVER_FEATURE_TESTSUITE=OFF \
          -DUSERVER_FEATURE_FORMAT=OFF \
          .. && \
    make -j2 && \
    cp /app/build/mail-lab /app/mail-lab && \
    chmod +x /app/mail-lab && \
    mkdir -p /app/data

HEALTHCHECK --interval=20s --timeout=5s --start-period=15s --retries=5 \
    CMD wget --quiet --tries=1 --spider http://127.0.0.1:8080/ping || exit 1

CMD ["/app/mail-lab", "--config", "/app/configs/static_config.yaml"]
