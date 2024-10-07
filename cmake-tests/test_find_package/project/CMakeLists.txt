cmake_minimum_required(VERSION 3.19)

project(UseFindPackageTest)

find_package(launchdarkly REQUIRED)

add_executable(use_find_package_server main_server.cpp)
target_link_libraries(use_find_package_server launchdarkly::server)


add_executable(use_find_package_client main_client.cpp)
target_link_libraries(use_find_package_client launchdarkly::client)
