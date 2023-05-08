#!/bin/bash -e

# This script builds a specific cmake target.
# This script should be ran from the root directory of the project.
# ./scripts/build.sh my-build-target ON
#
# $1 the name of the target. For example "launchdarkly-cpp-common".
# $2 ON/OFF which enables/disables building in a test configuration.

function cleanup {
  cd ..
}

mkdir -p build
cd build
# After we enter the directory we want to make sure we always exit it when the
# script ends.
trap cleanup EXIT

cmake -D BUILD_TESTING="$2" ..

cmake --build . --target "$1"
