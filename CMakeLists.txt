# This project aims to follow modern cmake guidelines, e.g.
# https://cliutils.gitlab.io/modern-cmake

# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyCPPSDKs
        VERSION 0.1
        DESCRIPTION "LaunchDarkly C++ SDK Monorepo (Server/Client)"
        LANGUAGES CXX C
)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24")
    # Affects robustness of timestamp checking on FetchContent dependencies.
    cmake_policy(SET CMP0135 NEW)
endif()

# All projects in this repo should share the same version of 3rd party depends.
# It's the only way to remain sane.
set(CMAKE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(libs/server-sent-events)
add_subdirectory(libs/client-sdk)
add_subdirectory(bindings/c)
add_subdirectory(apps)
