#!/bin/bash -e

# This script builds a specific cmake target.
# This script should be ran from the root directory of the project.
# ./scripts/build.sh my-build-target ON
#
# $1 the name of the target. For example "launchdarkly-cpp-common".
# $2 ON/OFF which enables/disables building in a test configuration (unit tests + contract tests.)

function cleanup {
  cd ..
}

mkdir -p build
cd build
# After we enter the directory we want to make sure we always exit it when the
# script ends.
trap cleanup EXIT

# Special case: unlike the other targets, enabling redis support will pull in redis++ and hiredis dependencies at
# configuration time. To ensure this only happens when asked, disable the support by default.
build_redis="OFF"
if [ "$1" == "launchdarkly-cpp-server-redis-source" ] || [ "$1" == "gtest_launchdarkly-cpp-server-redis-source" ]; then
  build_redis="ON"
fi



# Special case: when building for macOS, we need to specify the architectures to build for.
osx_archs="x86_64"

if [[ "$(uname -m)" == "arm64" ]]; then
    osx_archs="arm64;x86_64"
fi


cmake -G Ninja -D CMAKE_COMPILE_WARNING_AS_ERROR=TRUE \
               -D BUILD_TESTING="$2" \
               -D LD_BUILD_UNIT_TESTS="$2" \
               -D LD_BUILD_CONTRACT_TESTS="$2" \
               -D CMAKE_OSX_ARCHITECTURES="$osx_archs" \
               -D LD_BUILD_REDIS_SUPPORT="$build_redis" ..

cmake --build . --target "$1"
