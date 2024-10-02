#!/bin/bash -e


if [ -z "$BOOST_ROOT" ]; then
  echo "BOOST_ROOT is not set. Please set it to the root directory boost so it can be forwarded to CMake integration
  tests."
  exit 1
fi

if [ -z "$OPENSSL_ROOT_DIR" ]; then
  echo "OPENSSL_ROOT_DIR is not set. Please set it to the root directory of OpenSSL so it can be forwarded to CMake
  integration tests."
  exit 1
fi

function cleanup {
  cd ..
}

mkdir -p build
cd build
# After we enter the directory we want to make sure we always exit it when the
# script ends.
trap cleanup EXIT



cmake -G Ninja -D CMAKE_COMPILE_WARNING_AS_ERROR=TRUE \
               -D BUILD_TESTING=ON \
               -D LD_CMAKE_INTEGRATION_TESTS=ON \
               -D BOOST_ROOT="$BOOST_ROOT" \
               -D OPENSSL_ROOT_DIR="$OPENSSL_ROOT_DIR" \
               -D LD_BUILD_EXAMPLES=OFF ..
