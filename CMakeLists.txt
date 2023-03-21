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

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(libs/server-sent-events)
add_subdirectory(libs/client-sdk)
add_subdirectory(bindings/c)
add_subdirectory(apps)
