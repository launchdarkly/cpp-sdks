# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyHelloCPPServerRedisSource
        VERSION 0.1
        DESCRIPTION "LaunchDarkly Hello CPP Server-side SDK with Redis source"
        LANGUAGES CXX
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(hello-cpp-server-redis-source main.cpp)
target_link_libraries(hello-cpp-server-redis-source PRIVATE launchdarkly::server Threads::Threads)
