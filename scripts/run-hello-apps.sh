#!/bin/bash
# This script builds a set of hello-app cmake targets
# using static or dynamic linkage.
# This script should be ran from the root directory of the project.
# Example:
# ./scripts/run-hello-apps.sh dynamic hello-cpp-client hello-c-client
#
# $1 is the linkage, either 'static' or 'dynamic'.
# Subsequent arguments are cmake target names.

if [ -z "$1" ]
then
  echo "Linkage must be specified ('static' or 'dynamic')"
  exit 1
fi

dynamic_linkage="Off"
if [ "$1" == "dynamic" ]; then
    dynamic_linkage="On"
fi

shift

function cleanup {
  cd ..
}

mkdir -p build
cd build || exit

# After we enter the directory we want to make sure we always exit it when the
# script ends.
trap cleanup EXIT

cmake -G Ninja -D CMAKE_BUILD_TYPE=Release  -D BUILD_TESTING=OFF  -D LD_BUILD_SHARED_LIBS=$dynamic_linkage ..

cmake --build .

export LD_MOBILE_KEY="bogus"
export LD_SDK_KEY="bogus"

for target in "$@"
do
  ./examples/"$target"/"$target" | tee "$target"_output.txt
  grep "is true for this user" "$target"_output.txt || (echo "$target: expected flag to be true" && exit 1)
done
