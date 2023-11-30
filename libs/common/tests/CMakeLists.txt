cmake_minimum_required(VERSION 3.10)

include(GoogleTest)

include_directories("${PROJECT_SOURCE_DIR}/include")

file(GLOB tests "${PROJECT_SOURCE_DIR}/tests/*.cpp"
        "${PROJECT_SOURCE_DIR}/tests/bindings/*.cpp")


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Get things in the same directory on windows.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}../")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}../")

add_executable(gtest_${LIBNAME}
        ${tests})
target_link_libraries(gtest_${LIBNAME} launchdarkly::common launchdarkly::internal foxy GTest::gtest_main)
# There's an uninitialized variable warning in gtest that hasn't been resolved.
set_target_properties(gtest_${LIBNAME} PROPERTIES COMPILE_WARNING_AS_ERROR OFF)

gtest_discover_tests(gtest_${LIBNAME})
