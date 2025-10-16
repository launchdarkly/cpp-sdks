cmake_minimum_required(VERSION 3.10)

include(GoogleTest)

include_directories("${PROJECT_SOURCE_DIR}/include")

file(GLOB tests "${PROJECT_SOURCE_DIR}/tests/*.cpp"
        "${PROJECT_SOURCE_DIR}/tests/bindings/*.cpp")


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Get things in the same directory on windows.
if (WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}../")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}../")
endif ()

add_executable(gtest_${LIBNAME}
        ${tests})
target_link_libraries(gtest_${LIBNAME} launchdarkly::common launchdarkly::internal foxy GTest::gtest_main)

if (LD_CURL_NETWORKING)
    target_link_libraries(gtest_${LIBNAME} launchdarkly::networking)
endif()

gtest_discover_tests(gtest_${LIBNAME})
