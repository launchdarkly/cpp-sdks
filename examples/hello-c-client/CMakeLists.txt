# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyHelloCClient
        VERSION 0.1
        DESCRIPTION "LaunchDarkly Hello C Client"
        LANGUAGES C
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(hello-c main.c)
target_link_libraries(hello-c PRIVATE launchdarkly::client Threads::Threads)
