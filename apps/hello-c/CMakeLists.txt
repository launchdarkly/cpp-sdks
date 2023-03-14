# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyHelloCClient
        VERSION 0.1
        DESCRIPTION "LaunchDarkly Hello C Client"
        LANGUAGES CXX
)

add_executable(hello-c main.c)
target_link_libraries(hello-c PRIVATE launchdarkly::client-c)
