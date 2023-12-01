# This project aims to follow modern cmake guidelines, e.g.
# https://cliutils.gitlab.io/modern-cmake

# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)
include(CMakeDependentOption)

project(
        LaunchDarklyCPPSDKs
        VERSION 0.1
        DESCRIPTION "LaunchDarkly C++ SDK Monorepo (Server/Client)"
        LANGUAGES CXX C
)

include(GNUInstallDirs)

option(BUILD_TESTING "Top-level switch for testing. Turn off to disable unit and contract tests." ON)

option(LD_BUILD_SHARED_LIBS "Build the SDKs as shared libraries" OFF)

cmake_dependent_option(LD_DYNAMIC_LINK_BOOST
        "Dynamically link boost instead of building with its static libraries"
        ON  # Default to dynamically linking boost, because we can't provide static libs on all platforms that
        # are position-independent.
        "LD_BUILD_SHARED_LIBS" # only relevant if building SDK as shared lib
        OFF # If not building shared libs, then static link boost instead.
)

cmake_dependent_option(LD_BUILD_UNIT_TESTS
        "Build the C++ unit tests."
        ON                                        # default to enabling unit tests
        "BUILD_TESTING;NOT LD_BUILD_SHARED_LIBS"  # only exposed if top-level switch is on, and also only when building
        # static libs. This is because we have hidden visibility of symbols by default (to only expose our C API.)
        OFF                                       # otherwise, off
)

# If you want to run the unit tests with valgrind, then LD_TESTING_SANITIZERS must of OFF.
cmake_dependent_option(LD_TESTING_SANITIZERS
        "Enable sanitizers for unit tests."
        ON                      # default to enabling sanitizers
        "LD_BUILD_UNIT_TESTS"   # only expose if unit tests enabled..
        OFF                     # otherwise, off
)

cmake_dependent_option(LD_BUILD_CONTRACT_TESTS
        "Build contract test service."
        OFF                                         # default to disabling contract tests, since they require running a service
        "BUILD_TESTING;NOT LD_BUILD_SHARED_LIBS"    # only expose if top-level switch is on and using static libs, since C++ symbols needed would be hidden.
        OFF                                         # otherwise, off
)

# The general strategy is to produce a fat artifact containing all of our dependencies so users
# only have a single thing to link. We should support this either being a static or shared library.
# Because OpenSSL is a large, and security relevant dependency, we should have a separate option
# to link against that statically or dynamically.

option(LD_DYNAMIC_LINK_OPENSSL
        "Dynamically link OpenSSL instead of building with its static library"
        OFF # default to linking OpenSSL statically
)

option(LD_BUILD_EXAMPLES "Build hello-world examples." ON)

option(LD_BUILD_REDIS_SUPPORT "Build redis support." OFF)

# If using 'make' as the build system, CMake causes the 'install' target to have a dependency on 'all', meaning
# it will cause a full build. This disables that, allowing us to build piecemeal instead. This is useful
# so that we only need to build the client or server for a given release (if only the client or server were affected.)
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)

# All projects in this repo should share the same version of 3rd party depends.
# It's the only way to remain sane.
set(CMAKE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (LD_BUILD_UNIT_TESTS)
    message(STATUS "LaunchDarkly: building unit tests")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG")
    add_compile_definitions(LAUNCHDARKLY_USE_ASSERT)
    if (LD_TESTING_SANITIZERS)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=undefined -fsanitize=leak")
        elseif (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
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
    set_target_properties(gtest PROPERTIES COMPILE_WARNING_AS_ERROR OFF)


endif ()

if (LD_DYNAMIC_LINK_OPENSSL)
    message(STATUS "LaunchDarkly: searching for shared OpenSSL library")
    set(OPENSSL_USE_STATIC_LIBS OFF)
else ()
    message(STATUS "LaunchDarkly: searching for static OpenSSL library")
    set(OPENSSL_USE_STATIC_LIBS ON)
endif ()

find_package(OpenSSL REQUIRED)
message(STATUS "LaunchDarkly: using OpenSSL v${OPENSSL_VERSION}")


if (LD_DYNAMIC_LINK_BOOST)
    message(STATUS "LaunchDarkly: searching for shared Boost libraries")
    set(Boost_USE_STATIC_LIBS OFF)
else ()
    message(STATUS "LaunchDarkly: searching for static Boost libraries")
    set(Boost_USE_STATIC_LIBS ON)
endif ()

if (LD_BUILD_SHARED_LIBS)
    # When building a shared library we hide all symbols
    # aside from this we have specifically exported for the C-API.
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
endif ()

set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.81 REQUIRED COMPONENTS json url coroutine)
message(STATUS "LaunchDarkly: using Boost v${Boost_VERSION}")

include(${CMAKE_FILES}/certify.cmake)
add_subdirectory(vendor/foxy)

# Common, internal, and server-sent-events are built as "object" libraries.
add_subdirectory(libs/common)
add_subdirectory(libs/internal)
add_subdirectory(libs/server-sent-events)

if (LD_BUILD_REDIS_SUPPORT)
    message(STATUS "LaunchDarkly: building redis support")
    include(${CMAKE_FILES}/redis-plus-plus.cmake)
endif ()

# Built as static or shared depending on LD_BUILD_SHARED_LIBS variable.
# This target "links" in common, internal, and sse as object libraries.
add_subdirectory(libs/client-sdk)
add_subdirectory(libs/server-sdk)

if (LD_BUILD_CONTRACT_TESTS)
    message(STATUS "LaunchDarkly: building contract tests")
    add_subdirectory(contract-tests)
endif ()

if (LD_BUILD_EXAMPLES)
    message(STATUS "LaunchDarkly: building examples")
    add_subdirectory(examples)
endif ()
