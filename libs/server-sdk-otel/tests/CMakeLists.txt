# Tests for LaunchDarkly OpenTelemetry integration

file(GLOB SOURCES "*.cpp")

# Place test executable in the build root directory to match CI expectations
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Get things in the same directory on windows.
if (WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}../")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}../")
endif ()

add_executable(gtest_launchdarkly-cpp-server-otel ${SOURCES})

target_link_libraries(gtest_launchdarkly-cpp-server-otel
        PRIVATE
        ${LIBNAME}
        launchdarkly::server
        GTest::gtest
        GTest::gtest_main
        opentelemetry-cpp::api
        opentelemetry_trace  # SDK needed for test fixtures
)

target_include_directories(gtest_launchdarkly-cpp-server-otel
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

gtest_discover_tests(gtest_launchdarkly-cpp-server-otel)
