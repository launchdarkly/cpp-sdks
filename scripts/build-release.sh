#!/bin/bash -e

# Call this script with a CMakeTarget
# ./scripts/build-release launchdarkly-cpp-client

set -e

# Build a static release.
mkdir -p build-static && cd build-static
mkdir -p release
cmake -G Ninja -D CMAKE_BUILD_TYPE=Release -D BUILD_TESTING=OFF -D CMAKE_INSTALL_PREFIX=./release ..

cmake --build . --target "$1"
cmake --install .
cd ..

 Build a dynamic release.
mkdir -p build-dynamic && cd build-dynamic
mkdir -p release
cmake -G Ninja -D CMAKE_BUILD_TYPE=Release -D BUILD_TESTING=OFF -D BUILD_SHARED_LIBS=ON -D CMAKE_INSTALL_PREFIX=./release ..

cmake --build . --target "$1"
cmake --install .
cd ..
