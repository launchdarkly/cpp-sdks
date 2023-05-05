cmake_minimum_required(VERSION 3.10)

include(GoogleTest)

include_directories("${PROJECT_SOURCE_DIR}/include")

file(GLOB tests "${PROJECT_SOURCE_DIR}/tests/*.cpp")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Get things in the same directory on windows.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}../")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}../")

add_executable(gtest_${LIBNAME}
        ${tests})
target_link_libraries(gtest_${LIBNAME} launchdarkly::common GTest::gtest_main)

gtest_discover_tests(gtest_${LIBNAME})
