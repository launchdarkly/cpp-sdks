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


set(BUILD_TESTING OFF)
FetchContent_MakeAvailable(foxy)
set(BUILD_TESTING ON)
