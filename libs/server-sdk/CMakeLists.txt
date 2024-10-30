# This project aims to follow modern cmake guidelines, e.g.
# https://cliutils.gitlab.io/modern-cmake

# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyCPPServer
        VERSION 3.7.0 # {x-release-please-version}
        DESCRIPTION "LaunchDarkly C++ Server SDK"
        LANGUAGES CXX C
)

set(LIBNAME "launchdarkly-cpp-server")

# If this project is the main CMake project (as opposed to being included via add_subdirectory)
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Disable C++ extensions for portability.
    set(CMAKE_CXX_EXTENSIONS OFF)
    # Enable folder support in IDEs.
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif ()

# Needed to fetch external dependencies.
include(FetchContent)

# Needed to parse RFC3339 dates in flag rules.
include(${CMAKE_FILES}/rfc3339_timestamp.cmake)

# Add main SDK sources.
add_subdirectory(src)

if (LD_BUILD_UNIT_TESTS)
    add_subdirectory(tests)
endif ()
