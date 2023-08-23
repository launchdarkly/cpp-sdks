# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyHelloCPPClient
        VERSION 0.1
        DESCRIPTION "LaunchDarkly Hello CPP Client-side SDK"
        LANGUAGES CXX
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(hello-cpp-client main.cpp)
target_link_libraries(hello-cpp-client PRIVATE launchdarkly::client Threads::Threads)
