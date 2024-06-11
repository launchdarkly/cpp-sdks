# This project aims to follow modern cmake guidelines, e.g.
# https://cliutils.gitlab.io/modern-cmake

# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyCPPServerRedisSource
        VERSION 2.1.9 # {x-release-please-version}
        DESCRIPTION "LaunchDarkly C++ Server SDK Redis Source"
        LANGUAGES CXX C
)

set(LIBNAME "launchdarkly-cpp-server-redis-source")

# If this project is the main CMake project (as opposed to being included via add_subdirectory)
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Disable C++ extensions for portability.
    set(CMAKE_CXX_EXTENSIONS OFF)
    # Enable folder support in IDEs.
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif ()

# Needed to fetch external dependencies.
include(FetchContent)

set(REDIS_PLUS_PLUS_BUILD_SHARED OFF CACHE BOOL "" FORCE)

include(${CMAKE_FILES}/redis-plus-plus.cmake)

add_subdirectory(src)

if (LD_BUILD_UNIT_TESTS)
    add_subdirectory(tests)
endif ()
