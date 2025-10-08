#!/bin/bash
# This script builds a set of hello-app cmake targets
# using static or dynamic linkage.
# This script should be ran from the root directory of the project.
# Example:
# ./scripts/run-hello-apps.sh dynamic hello-cpp-client hello-c-client
#
# $1 is the linkage, either 'static', 'dynamic', or 'dynamic-export-all-symbols'.
# The difference between the dynamic options is that 'dynamic' only exports the C API,
# whereas 'dynamic-export-all-symbols' exports all symbols (including C++, so we can link
# a dynamic C++ library in the hello example.)
# Subsequent arguments are cmake target names.

if [ "$1" != "static" ] && [ "$1" != "dynamic" ] && [ "$1" != "dynamic-export-all-symbols" ]
then
  echo "Linkage must be specified ('static', 'dynamic', or 'dynamic-export-all-symbols')"
  exit 1
fi

dynamic_linkage="Off"
export_all_symbols="Off"

if [ "$1" == "dynamic" ]; then
    dynamic_linkage="On"
elif [ "$1" == "dynamic-export-all-symbols" ]; then
    dynamic_linkage="On"
    export_all_symbols="On"
fi

shift

function cleanup {
  cd ..
}

mkdir -p build-"$1"
cd build-"$1" || exit

# After we enter the directory we want to make sure we always exit it when the
# script ends.
trap cleanup EXIT

echo Boost dir $Boost_DIR

cmake -G Ninja -D CMAKE_BUILD_TYPE=Release \
               -D BUILD_TESTING=OFF  \
               -D LD_BUILD_SHARED_LIBS=$dynamic_linkage \
               -D LD_BUILD_EXPORT_ALL_SYMBOLS=$export_all_symbols ..

export LD_MOBILE_KEY="bogus"
export LD_SDK_KEY="bogus"

for target in "$@"
do
  cmake --build . --target "$target"
  ./examples/"$target"/"$target" | tee "$target"_output.txt
  grep -F "*** SDK" "$target"_output.txt || (echo "$target: expected connection to LD to fail" && exit 1)
done
