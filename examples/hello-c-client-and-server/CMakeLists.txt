# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyHelloCClientAndServer
        VERSION 0.1
        DESCRIPTION "LaunchDarkly Hello C Client-side and Server-side SDK"
        LANGUAGES C
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(hello-c-client-and-server main.c)
target_link_libraries(hello-c-client-and-server PRIVATE launchdarkly::client launchdarkly::server launchdarkly::sse launchdarkly::common Threads::Threads)
