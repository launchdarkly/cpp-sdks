#!/bin/bash -e

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
               -D LD_BUILD_EXAMPLES=OFF ..

cmake --build .
