#!/bin/bash -e

# Call this script with a CMakeTarget
# ./scripts/build-release launchdarkly-cpp-client

# Build standard configuration
source ./scripts/build-release.sh

# Build a static debug release.

mkdir -p build-static-debug && cd build-static-debug
mkdir -p release
cmake -G Ninja -D CMAKE_BUILD_TYPE=Debug -D BUILD_TESTING=OFF -D CMAKE_INSTALL_PREFIX=./release ..

cmake --build . --target "$1"

cmake --build . --target "$1"
cmake --install .

cd ..

# Build a dynamic debug release.
mkdir -p build-dynamic-debug && cd build-dynamic-debug
mkdir -p release
cmake -G Ninja -D CMAKE_BUILD_TYPE=Debug -D BUILD_TESTING=OFF -D BUILD_SHARED_LIBS=ON -D CMAKE_INSTALL_PREFIX=./release ..

cmake --build . --target "$1"
cmake --install .

cmake --build . --target "$1"
