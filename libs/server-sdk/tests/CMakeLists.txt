cmake_minimum_required(VERSION 3.10)
include(GoogleTest)

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/src")

file(GLOB tests "${PROJECT_SOURCE_DIR}/tests/*.cpp")

if (LD_BUILD_REDIS_SUPPORT)
    file(GLOB redis_tests "${PROJECT_SOURCE_DIR}/tests/redis/*.cpp")
    list(APPEND tests ${redis_tests})
endif ()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Get things in the same directory on windows.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}../")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}../")

add_executable(gtest_${LIBNAME}
        ${tests}
)

target_link_libraries(gtest_${LIBNAME} PRIVATE launchdarkly::server launchdarkly::internal launchdarkly::sse timestamp GTest::gmock_main)

if (LD_BUILD_REDIS_SUPPORT)
    target_link_libraries(gtest_${LIBNAME} PRIVATE redis++::redis++_static)
endif ()

gtest_discover_tests(gtest_${LIBNAME})
