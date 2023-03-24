cmake_minimum_required(VERSION 3.10)
include(GoogleTest)

include_directories("${PROJECT_SOURCE_DIR}/include")

file(GLOB tests "${PROJECT_SOURCE_DIR}/tests/*.cpp")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(gtest_launchdarkly-cpp-common
        ${tests})
target_link_libraries(gtest_launchdarkly-cpp-common launchdarkly::common GTest::gtest_main)

gtest_discover_tests(gtest_launchdarkly-cpp-common)
