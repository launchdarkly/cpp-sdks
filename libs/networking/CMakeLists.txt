cmake_minimum_required(VERSION 3.19)

project(
    LaunchDarklyNetworking
    VERSION 0.1.0
    DESCRIPTION "LaunchDarkly C++ Networking Library"
    LANGUAGES CXX
)

set(LIBNAME "launchdarkly-cpp-networking")

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif ()

add_subdirectory(src)
