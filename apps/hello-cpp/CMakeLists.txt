# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyHelloCPPClient
        VERSION 0.1
        DESCRIPTION "LaunchDarkly Hello CPP Client"
        LANGUAGES CXX
)

add_executable(hello-cpp main.cpp)
target_link_libraries(hello-cpp PRIVATE launchdarkly::client launchdarkly::sse)
