#!/bin/bash -e

# Call this script with a CMakeTarget and optional flags
# ./scripts/build-release.sh launchdarkly-cpp-client
# ./scripts/build-release.sh launchdarkly-cpp-client --with-curl

set -e

# Parse arguments
TARGET="$1"
build_redis="OFF"
build_curl="OFF"

# Special case: unlike the other targets, enabling redis support will pull in redis++ and hiredis dependencies at
# configuration time. To ensure this only happens when asked, disable the support by default.
if [ "$TARGET" == "launchdarkly-cpp-server-redis-source" ]; then
  build_redis="ON"
fi

# Check for --with-curl flag
for arg in "$@"; do
  if [ "$arg" == "--with-curl" ]; then
    build_curl="ON"
    break
  fi
done

# Determine suffix for build directories
if [ "$build_curl" == "ON" ]; then
  suffix="-curl"
else
  suffix=""
fi

# Build a static release.
mkdir -p "build-static${suffix}" && cd "build-static${suffix}"
mkdir -p release
cmake -G Ninja -D CMAKE_BUILD_TYPE=Release -D LD_BUILD_REDIS_SUPPORT="$build_redis" -D LD_CURL_NETWORKING="$build_curl" -D BUILD_TESTING=OFF -D CMAKE_INSTALL_PREFIX=./release ..

cmake --build . --target "$TARGET"
cmake --install .
cd ..

# Build a dynamic release.
mkdir -p "build-dynamic${suffix}" && cd "build-dynamic${suffix}"
mkdir -p release
cmake -G Ninja -D CMAKE_BUILD_TYPE=Release -D LD_BUILD_REDIS_SUPPORT="$build_redis" -D LD_CURL_NETWORKING="$build_curl" -D BUILD_TESTING=OFF -D LD_BUILD_SHARED_LIBS=ON -D CMAKE_INSTALL_PREFIX=./release ..

cmake --build . --target "$TARGET"
cmake --install .
cd ..
