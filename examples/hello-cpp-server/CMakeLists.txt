# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyHelloCPPServer
        VERSION 0.1
        DESCRIPTION "LaunchDarkly Hello CPP Server-side SDK"
        LANGUAGES CXX
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(hello-cpp-server main.cpp)
target_link_libraries(hello-cpp-server PRIVATE launchdarkly::server Threads::Threads)
