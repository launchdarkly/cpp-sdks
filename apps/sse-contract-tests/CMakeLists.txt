# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyCPPSSETestHarness
        VERSION 0.1
        DESCRIPTION "LaunchDarkly CPP SSE Test Harness"
        LANGUAGES CXX
)

include(${CMAKE_FILES}/json.cmake)

add_executable(sse-tests
        src/main.cpp
        src/server.cpp
        src/entity_manager.cpp
        src/session.cpp
        src/stream_entity.cpp
)

target_link_libraries(sse-tests PRIVATE
        launchdarkly::sse
        nlohmann_json::nlohmann_json
)

target_include_directories(sse-tests PUBLIC include)
