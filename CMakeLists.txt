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

option(BUILD_TESTING "Enable C++ unit tests." ON)

if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
endif()


add_subdirectory(libs/common)
add_subdirectory(libs/server-sent-events)
add_subdirectory(libs/client-sdk)
add_subdirectory(bindings/c)
add_subdirectory(apps)
