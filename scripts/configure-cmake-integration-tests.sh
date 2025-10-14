#!/bin/bash -e

function cleanup {
  cd ..
}

mkdir -p build
cd build
# After we enter the directory we want to make sure we always exit it when the
# script ends.
trap cleanup EXIT

echo "=== CMake Configuration Parameters ==="
echo "BOOST_ROOT: $BOOST_ROOT"
echo "OPENSSL_ROOT_DIR: $OPENSSL_ROOT_DIR"
echo "CMAKE_INSTALL_PREFIX: $CMAKE_INSTALL_PREFIX"
echo "CMAKE_EXTRA_ARGS: $CMAKE_EXTRA_ARGS"
echo "CURL_ROOT: $CURL_ROOT"
echo "CMAKE_PREFIX_PATH: $CMAKE_PREFIX_PATH"
echo "======================================="

cmake -G Ninja -D CMAKE_COMPILE_WARNING_AS_ERROR=TRUE \
               -D BUILD_TESTING=ON \
               -D LD_BUILD_UNIT_TESTS=ON \
               -D LD_CMAKE_INTEGRATION_TESTS=ON \
               -D BOOST_ROOT="$BOOST_ROOT" \
               -D OPENSSL_ROOT_DIR="$OPENSSL_ROOT_DIR" \
               -D LD_TESTING_SANITIZERS=OFF \
               -D CMAKE_INSTALL_PREFIX="$CMAKE_INSTALL_PREFIX" \
               -D LD_BUILD_EXAMPLES=OFF \
               ${CMAKE_EXTRA_ARGS} \
               ..
