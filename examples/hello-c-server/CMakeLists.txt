# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyHelloCServer
        VERSION 0.1
        DESCRIPTION "LaunchDarkly Hello C Server-side SDK"
        LANGUAGES C
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(hello-c-server main.c)
target_link_libraries(hello-c-server PRIVATE launchdarkly::server launchdarkly::sse launchdarkly::common Threads::Threads)
