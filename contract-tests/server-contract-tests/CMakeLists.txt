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
        src/entity_manager.cpp
        src/client_entity.cpp
        src/contract_test_hook.cpp
        )

target_link_libraries(server-tests PRIVATE
        launchdarkly::server
        launchdarkly::internal
        foxy
        nlohmann_json::nlohmann_json
        Boost::coroutine
        contract-test-data-model
        )

if (LD_BUILD_REDIS_SUPPORT)
    target_link_libraries(server-tests PRIVATE launchdarkly::server_redis_source)
    target_compile_definitions(server-tests PRIVATE LD_REDIS_SUPPORT_ENABLED)
endif ()

target_include_directories(server-tests PUBLIC include)
