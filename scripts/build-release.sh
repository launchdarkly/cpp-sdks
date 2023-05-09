# Build a static release.
# Call this script with a CMakeTarget
# ./scripts/build-release launchdarkly-cpp-client

#-D CMAKE_BUILD_TYPE=Release

mkdir -p build-static && cd build-static
mkdir -p release
cmake -G Ninja -D BUILD_TESTING=OFF -D CMAKE_INSTALL_PREFIX=./release ..

cmake --build . --target "$1"
cd ..

# Build a dynamic release.
mkdir -p build-dynamic && cd build-dynamic
mkdir -p release
cmake -G Ninja -D BUILD_TESTING=OFF -D BUILD_SHARED_LIBS=ON -D CMAKE_INSTALL_PREFIX=./release ..

cmake --build . --target "$1"
