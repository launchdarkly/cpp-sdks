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
        ${tests}
)

set(LIBS
        launchdarkly::server_redis_source
        # Needed so we can access the flag/segment data models so we can serialize them for putting in redis.
        launchdarkly::internal
        # Needed because the source doesn't (need to) expose redis++ as a public dependency, but we need to construct
        # a redis client in the tests.
        redis++::redis++_static
        GTest::gtest_main
        GTest::gmock
)

target_link_libraries(gtest_${LIBNAME} PRIVATE ${LIBS})

# There's an uninitialized variable warning in gtest that hasn't been resolved.
set_target_properties(gtest_${LIBNAME} PROPERTIES COMPILE_WARNING_AS_ERROR OFF)

gtest_discover_tests(gtest_${LIBNAME})
