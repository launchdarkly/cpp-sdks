# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyCPPClientSDKTestHarness
        VERSION 0.1
        DESCRIPTION "LaunchDarkly CPP Client-side SDK Test Harness"
        LANGUAGES CXX
)

include(${CMAKE_FILES}/json.cmake)

add_executable(client-tests
        src/main.cpp
        src/server.cpp
        src/session.cpp
        src/entity_manager.cpp
        src/client_entity.cpp
        )

target_link_libraries(client-tests PRIVATE
        launchdarkly::client
        launchdarkly::internal
        foxy
        nlohmann_json::nlohmann_json
        Boost::coroutine
        contract-test-data-model
        )

if (LD_CURL_NETWORKING)
        target_compile_definitions(client-tests PUBLIC LD_CURL_NETWORKING)
endif()

target_include_directories(client-tests PUBLIC include)
