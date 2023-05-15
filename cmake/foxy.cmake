cmake_minimum_required(VERSION 3.11)

include(FetchContent)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24")
    # Affects robustness of timestamp checking on FetchContent dependencies.
    cmake_policy(SET CMP0135 NEW)
endif ()


FetchContent_Declare(foxy
        GIT_REPOSITORY https://github.com/launchdarkly/foxy.git
        GIT_TAG rlamb/skip-install
        )


set(ORIGINAL_BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS}")
set(ORIGINAL_BUILD_TESTING "${BUILD_TESTING}")

set(BUILD_TESTING OFF)
set(BUILD_SHARED_LIBS OFF)
set(FOXY_SKIP_INSTALL ON)
FetchContent_MakeAvailable(foxy)

set(BUILD_TESTING "${ORIGINAL_BUILD_TESTING}")
set(BUILD_SHARED_LIBS "${ORIGINAL_BUILD_SHARED_LIBS}")
