cmake_minimum_required(VERSION 3.11)

include(FetchContent)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24")
    # Affects robustness of timestamp checking on FetchContent dependencies.
    cmake_policy(SET CMP0135 NEW)
endif ()


FetchContent_Declare(foxy
        GIT_REPOSITORY https://github.com/launchdarkly/foxy.git
        GIT_TAG 7f4ac0495ad2ed9cd0eca5994743d677ac1d2636
        )

# Disable testing for foxy; can be enabled if we bring in Catch2 dependency.
get_property(ORIGINAL_BUILD_TESTING GLOBAL PROPERTY BUILD_TESTING)
set(BUILD_TESTING OFF CACHE BOOL "Disable building tests for foxy" FORCE)
FetchContent_MakeAvailable(foxy)
set_property(GLOBAL PROPERTY BUILD_TESTING "${ORIGINAL_BUILD_TESTING}")
