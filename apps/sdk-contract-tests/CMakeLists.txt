# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyCPPSDKTestHarness
        VERSION 0.1
        DESCRIPTION "LaunchDarkly CPP SDK Test Harness"
        LANGUAGES CXX
)

include(${CMAKE_FILES}/json.cmake)
include(${CMAKE_FILES}/foxy.cmake)

add_executable(sdk-tests
        src/main.cpp
        src/server.cpp
        src/session.cpp
        src/definitions.cpp
        src/entity_manager.cpp
        src/client_entity.cpp
        )

target_link_libraries(sdk-tests PRIVATE
        launchdarkly::client
        launchdarkly::internal
        foxy
        nlohmann_json::nlohmann_json
        Boost::coroutine
        )

target_include_directories(sdk-tests PUBLIC include)
