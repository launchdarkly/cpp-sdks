cmake_minimum_required(VERSION 3.19)

project(UseFindPackageTest)

find_package(launchdarkly REQUIRED)

add_executable(use_find_package main.cpp)
target_link_libraries(use_find_package launchdarkly::server)
