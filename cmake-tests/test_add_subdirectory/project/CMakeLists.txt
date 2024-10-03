cmake_minimum_required(VERSION 3.19)

project(AddSubdirectoryTest)

add_subdirectory(
        # Source directory where the SDK's CMakeLists.txt is located.
        ${LAUNCHDARKLY_SOURCE_DIR}
        # Binary directory must be specified when using an out-of-tree source.
        ${CMAKE_CURRENT_BINARY_DIR}/launchdarkly
)

set(TARGET_PREFIX add_subdirectory)

# Server-side
add_executable(${TARGET_PREFIX}_server_cpp main_server.cpp)
target_link_libraries(${TARGET_PREFIX}_server_cpp launchdarkly-cpp-server)

add_executable(${TARGET_PREFIX}_server_cpp_alias main_server.cpp)
target_link_libraries(${TARGET_PREFIX}_server_cpp_alias launchdarkly::server)

# Client-side
add_executable(${TARGET_PREFIX}_client_cpp main_client.cpp)
target_link_libraries(${TARGET_PREFIX}_client_cpp launchdarkly-cpp-client)

add_executable(${TARGET_PREFIX}_client_cpp_alias main_client.cpp)
target_link_libraries(${TARGET_PREFIX}_client_cpp_alias launchdarkly::client)
