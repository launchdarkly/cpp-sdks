# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyCClientAndServerCoexistence
        VERSION 0.1
        DESCRIPTION "LaunchDarkly C Client-side and Server-side SDK coexistence in same application"
        LANGUAGES C
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(c-client-and-server main.c)
target_link_libraries(c-client-and-server PRIVATE launchdarkly::client launchdarkly::server launchdarkly::sse launchdarkly::common Threads::Threads)
