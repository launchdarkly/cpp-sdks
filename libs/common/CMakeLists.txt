# This project aims to follow modern cmake guidelines, e.g.
# https://cliutils.gitlab.io/modern-cmake

# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyCommonSdk
        VERSION 0.1
        DESCRIPTION "LaunchDarkly C++ Common SDK"
        LANGUAGES CXX C
)

set(LIBNAME "launchdarkly-cpp-common")

# If this project is the main CMake project (as opposed to being included via add_subdirectory)
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Disable C++ extensions for portability.
    set(CMAKE_CXX_EXTENSIONS OFF)
    # Enable folder support in IDEs.
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif ()

# Needed to fetch external dependencies.
include(FetchContent)

include(${CMAKE_FILES}/expected.cmake)
#include(${CMAKE_FILES}/foxy.cmake)

# Add main SDK sources.
add_subdirectory(src)

if (LD_BUILD_UNIT_TESTS)
    add_subdirectory(tests)
endif ()
