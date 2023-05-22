# This project aims to follow modern cmake guidelines, e.g.
# https://cliutils.gitlab.io/modern-cmake

# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklySSEClient
        VERSION 0.1
        DESCRIPTION "LaunchDarkly Server-Sent-Events Client"
        LANGUAGES CXX
)

set(LIBNAME "launchdarkly-sse-client")

# If this project is the main CMake project (as opposed to being included via add_subdirectory)
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Disable C++ extensions for portability.
    set(CMAKE_CXX_EXTENSIONS OFF)
    # Enable folder support in IDEs.
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif ()

#set(CMAKE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_FILES})

# Needed to fetch external dependencies.
include(FetchContent)

#include(${CMAKE_FILES}/foxy.cmake)

add_subdirectory(src)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()
