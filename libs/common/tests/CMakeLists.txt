cmake_minimum_required(VERSION 3.10)
include(GoogleTest)

include_directories("${PROJECT_SOURCE_DIR}/include")

file(GLOB tests "${PROJECT_SOURCE_DIR}/tests/*.cpp")

add_executable("google_tests_common"
        ${tests})
target_link_libraries(google_tests_common launchdarkly::common GTest::gtest_main)

gtest_discover_tests(google_tests_common)
