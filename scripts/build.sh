#!/bin/bash -e

# This script builds a specific cmake target.
# This script should be ran from the root directory of the project.
# ./scripts/build.sh my-build-target ON
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
# Special case: OpenTelemetry support requires additional dependencies.
# Enable OTEL support and fetch deps when building OTEL targets.
# Disable contract tests for OTEL builds to avoid dependency conflicts.
build_otel="OFF"
build_otel_fetch_deps="OFF"
build_contract_tests="$2"
if [ "$1" == "launchdarkly-cpp-server-otel" ] || [ "$1" == "gtest_launchdarkly-cpp-server-otel" ]; then
  build_otel="ON"
  build_otel_fetch_deps="ON"
fi

echo "==== Build Configuration ===="
echo "Target: $1"
echo "BUILD_TESTING: $2"
echo "LD_BUILD_UNIT_TESTS: $2"
echo "LD_BUILD_CONTRACT_TESTS: $2"
echo "LD_BUILD_REDIS_SUPPORT: $build_redis"
echo "LD_CURL_NETWORKING: $build_curl"
echo "LD_BUILD_OTEL_SUPPORT: $build_otel"
echo "LD_BUILD_OTEL_FETCH_DEPS: $build_otel_fetch_deps"
echo "============================="

cmake -G Ninja -D CMAKE_COMPILE_WARNING_AS_ERROR=TRUE \
               -D BUILD_TESTING="$2" \
               -D LD_BUILD_UNIT_TESTS="$2" \
               -D LD_BUILD_CONTRACT_TESTS="$2" \
               -D LD_BUILD_REDIS_SUPPORT="$build_redis" \
               -D LD_CURL_NETWORKING="$build_curl" \
               -D LD_BUILD_OTEL_SUPPORT="$build_otel" \
               -D LD_BUILD_OTEL_FETCH_DEPS="$build_otel_fetch_deps" ..

cmake --build . --target "$1"
