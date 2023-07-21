# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyCPPServerSDKTestHarness
        VERSION 0.1
        DESCRIPTION "LaunchDarkly CPP Server-side SDK Test Harness"
        LANGUAGES CXX
)

include(${CMAKE_FILES}/json.cmake)

add_executable(server-tests
        src/main.cpp
        src/server.cpp
        src/session.cpp
        src/definitions.cpp
        src/entity_manager.cpp
        src/client_entity.cpp
        )

target_link_libraries(server-tests PRIVATE
        launchdarkly::server
        launchdarkly::internal
        foxy
        nlohmann_json::nlohmann_json
        Boost::coroutine
        )

target_include_directories(server-tests PUBLIC include)
