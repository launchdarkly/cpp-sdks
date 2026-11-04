cmake_minimum_required(VERSION 3.10)
include(GoogleTest)

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/src")

file(GLOB tests "${PROJECT_SOURCE_DIR}/tests/*.cpp")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Get things in the same directory on windows.
if (WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}../")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}../")
endif ()

add_executable(gtest_${LIBNAME}
        ${tests})
target_link_libraries(gtest_${LIBNAME} launchdarkly::client launchdarkly::internal launchdarkly::sse GTest::gtest_main)

gtest_discover_tests(gtest_${LIBNAME})

# Compiled as C so that C++-only syntax cannot leak into a header that C
# consumers include.
add_library(c_headers_smoke_${LIBNAME} OBJECT
        "${PROJECT_SOURCE_DIR}/tests/c_headers_smoke_test.c")
target_link_libraries(c_headers_smoke_${LIBNAME} PRIVATE launchdarkly::client)
set_target_properties(c_headers_smoke_${LIBNAME} PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)
