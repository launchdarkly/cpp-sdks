#!/bin/bash -e

# This script builds a specific cmake target.
# This script should be ran from the root directory of the project.
# ./scripts/build.sh my-build-target ON [true|false]
#
# $1 the name of the target. For example "launchdarkly-cpp-common".
# $2 ON/OFF which enables/disables building in a test configuration (unit tests + contract tests.)
# $3 (optional) true/false to enable/disable CURL networking (LD_CURL_NETWORKING)

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

# Check for CURL networking option
build_curl="OFF"
if [ "$3" == "true" ]; then
  build_curl="ON"
fi

cmake -G Ninja -D CMAKE_COMPILE_WARNING_AS_ERROR=TRUE \
               -D BUILD_TESTING="$2" \
               -D LD_BUILD_UNIT_TESTS="$2" \
               -D LD_BUILD_CONTRACT_TESTS="$2" \
               -D LD_BUILD_REDIS_SUPPORT="$build_redis" \
               -D LD_CURL_NETWORKING="$build_curl" ..

cmake --build . --target "$1"
