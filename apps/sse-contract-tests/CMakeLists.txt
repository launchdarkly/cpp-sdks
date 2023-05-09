# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyCPPSSETestHarness
        VERSION 0.1
        DESCRIPTION "LaunchDarkly CPP SSE Test Harness"
        LANGUAGES CXX
)

if (BUILD_SHARED_LIBS)
    set(Boost_USE_STATIC_LIBS OFF)
else ()
    set(Boost_USE_STATIC_LIBS ON)
endif ()

set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.80 REQUIRED COMPONENTS coroutine)


include(${CMAKE_FILES}/json.cmake)
include(${CMAKE_FILES}/foxy.cmake)

add_executable(sse-tests
        src/main.cpp
        src/server.cpp
        src/entity_manager.cpp
        src/session.cpp
        src/event_outbox.cpp
        )

target_link_libraries(sse-tests PRIVATE
        launchdarkly::sse
        launchdarkly::common
        foxy
        nlohmann_json::nlohmann_json
        Boost::coroutine
        )

target_include_directories(sse-tests PUBLIC include)

#add_definitions(-DBOOST_ASIO_ENABLE_HANDLER_TRACKING)
