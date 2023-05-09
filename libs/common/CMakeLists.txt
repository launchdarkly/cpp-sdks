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

set(OPENSSL_USE_STATIC_LIBS ON)
set(OPENSSL_ROOT_DIR "/opt/homebrew/opt/openssl@1.1")
find_package(OpenSSL REQUIRED)
message(STATUS "LaunchDarkly: using OpenSSL v${OPENSSL_VERSION}")

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.80 REQUIRED COMPONENTS json url)
message(STATUS "LaunchDarkly: using Boost v${Boost_VERSION}")

include(${CMAKE_FILES}/expected.cmake)
include(${CMAKE_FILES}/foxy.cmake)

# Add main SDK sources.
add_subdirectory(src)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()
