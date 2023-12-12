#!/bin/bash -e

# Call this script with a CMakeTarget
# ./scripts/build-release launchdarkly-cpp-client

set -e

# Special case: unlike the other targets, enabling redis support will pull in redis++ and hiredis dependencies at
# configuration time. To ensure this only happens when asked, disable the support by default.
build_redis="OFF"
if [ "$1" == "launchdarkly-cpp-server-redis-source" ]; then
  build_redis="ON"
fi

# Build a static release.
mkdir -p build-static && cd build-static
mkdir -p release
cmake -G Ninja -D CMAKE_BUILD_TYPE=Release \
               -D LD_BUILD_REDIS_SUPPORT="$build_redis" \
               -D BUILD_TESTING=OFF \
               -D CMAKE_INSTALL_PREFIX=./release ..

cmake --build . --target "$1"
cmake --install .
cd ..

# Build a dynamic release.
mkdir -p build-dynamic && cd build-dynamic
mkdir -p release
cmake -G Ninja -D CMAKE_BUILD_TYPE=Release \
               -D LD_BUILD_REDIS_SUPPORT="$build_redis" \
               -D BUILD_TESTING=OFF \
               -D LD_BUILD_SHARED_LIBS=ON \
               -D LD_DYNAMIC_LINK_BOOST=OFF \
               -D CMAKE_INSTALL_PREFIX=./release ..

cmake --build . --target "$1"
cmake --install .
cd ..

# Build a static debug release.
mkdir -p build-static-debug && cd build-static-debug
mkdir -p release
cmake -G Ninja -D CMAKE_BUILD_TYPE=Debug \
               -D BUILD_TESTING=OFF \
               -D LD_BUILD_REDIS_SUPPORT="$build_redis" \
               -D CMAKE_INSTALL_PREFIX=./release ..

cmake --build . --target "$1"
cmake --install .

cd ..

# Build a dynamic debug release.
mkdir -p build-dynamic-debug && cd build-dynamic-debug
mkdir -p release
cmake -G Ninja -D CMAKE_BUILD_TYPE=Debug \
               -D BUILD_TESTING=OFF \
               -D LD_BUILD_REDIS_SUPPORT="$build_redis" \
               -D LD_BUILD_SHARED_LIBS=ON \
               -D LD_DYNAMIC_LINK_BOOST=OFF \
               -D CMAKE_INSTALL_PREFIX=./release ..

cmake --build . --target "$1"
cmake --install .
