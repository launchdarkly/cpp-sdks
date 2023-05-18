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

include(GNUInstallDirs)

# All projects in this repo should share the same version of 3rd party depends.
# It's the only way to remain sane.
set(CMAKE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_TESTING "Enable C++ unit tests." ON)

# If you want to run the unit tests with valgrind, then TESTING_SANITIZERS must of OFF.
option(TESTING_SANITIZERS "Enable sanitizers for unit tests." ON)

if (BUILD_TESTING)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG")
    if (TESTING_SANITIZERS)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=undefined -fsanitize=leak")
        elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=leak")
        endif ()
    endif ()
    include(FetchContent)
    if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24")
        # Affects robustness of timestamp checking on FetchContent dependencies.
        cmake_policy(SET CMP0135 NEW)
    endif ()
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
endif ()

set(OPENSSL_USE_STATIC_LIBS ON)
set(OPENSSL_ROOT_DIR "/opt/homebrew/opt/openssl@1.1")
find_package(OpenSSL REQUIRED)
message(STATUS "LaunchDarkly: using OpenSSL v${OPENSSL_VERSION}")

if (MSVC)
    set(Boost_USE_STATIC_LIBS ON)
else ()
    if (BUILD_SHARED_LIBS)
        set(Boost_USE_STATIC_LIBS OFF)
    else ()
        set(Boost_USE_STATIC_LIBS ON)
    endif ()
endif ()

set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.80 REQUIRED COMPONENTS json url coroutine)
message(STATUS "LaunchDarkly: using Boost v${Boost_VERSION}")

add_subdirectory(libs/client-sdk)

set(ORIGINAL_BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF)

# Always build the common libraries as shared libs.
add_subdirectory(libs/common)
add_subdirectory(libs/internal)
add_subdirectory(libs/server-sent-events)
add_subdirectory(bindings/c)

set(BUILD_SHARED_LIBS "${ORIGINAL_BUILD_SHARED_LIBS}")

add_subdirectory(apps)
