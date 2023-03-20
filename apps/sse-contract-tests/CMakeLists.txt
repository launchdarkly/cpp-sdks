# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyCPPSSETestHarness
        VERSION 0.1
        DESCRIPTION "LaunchDarkly CPP SSE Test Harness"
        LANGUAGES CXX
)

include(${CMAKE_FILES}/json.cmake)

add_executable(sse-tests main.cpp)
target_link_libraries(sse-tests PRIVATE
        launchdarkly::sse
        nlohmann_json::nlohmann_json
)
