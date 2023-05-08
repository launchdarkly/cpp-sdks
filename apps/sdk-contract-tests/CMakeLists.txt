# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyCPPSDKTestHarness
        VERSION 0.1
        DESCRIPTION "LaunchDarkly CPP SDK Test Harness"
        LANGUAGES CXX
)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.80 REQUIRED COMPONENTS coroutine)


include(${CMAKE_FILES}/json.cmake)
include(${CMAKE_FILES}/foxy.cmake)

add_executable(sdk-tests
        src/main.cpp
        src/server.cpp
        src/session.cpp
        src/definitions.cpp
        src/entity_manager.cpp
        )

target_link_libraries(sdk-tests PRIVATE
        launchdarkly::client
        foxy
        nlohmann_json::nlohmann_json
        Boost::coroutine
        )

target_include_directories(sdk-tests PUBLIC include)
