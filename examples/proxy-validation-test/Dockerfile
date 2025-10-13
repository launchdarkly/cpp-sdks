FROM ubuntu:24.04

# Install dependencies
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    libboost-all-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    git \
    curl \
    netcat-openbsd \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /sdk

# Copy SDK source
COPY . .

# Clean any existing build directory from host
RUN rm -rf build

# Build the SDK with CURL networking enabled
RUN cmake -B build -S . -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DLD_BUILD_SHARED_LIBS=OFF \
    -DLD_BUILD_EXAMPLES=ON \
    -DLD_BUILD_UNIT_TESTS=OFF \
    -DLD_CURL_NETWORKING=ON \
    && cmake --build build --target hello-cpp-client

COPY examples/proxy-validation-test/test-proxy.sh /test-proxy.sh
RUN chmod +x /test-proxy.sh

# Set entrypoint
ENTRYPOINT ["/test-proxy.sh"]
