cmake_minimum_required(VERSION 3.19)
include(GoogleTest)

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/src")

file(GLOB tests "${PROJECT_SOURCE_DIR}/tests/*.cpp")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if (WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}../")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}../")
endif ()

add_executable(gtest_${LIBNAME}
        ${tests}
)

set_target_properties(gtest PROPERTIES COMPILE_WARNING_AS_ERROR OFF)

set(LIBS
        launchdarkly::server_dynamodb_source
        # Needed for access to the data models (Flag, Segment) and their
        # JSON serializers, which the fixture uses to seed test data.
        launchdarkly::internal
        # Needed because the source doesn't expose the AWS SDK publicly, but
        # the test fixture talks to DynamoDB directly to put rows.
        aws-cpp-sdk-dynamodb
        aws-cpp-sdk-core
        GTest::gtest_main
        GTest::gmock
)

target_link_libraries(gtest_${LIBNAME} PRIVATE ${LIBS})

gtest_discover_tests(gtest_${LIBNAME})
