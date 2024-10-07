cmake_minimum_required(VERSION 3.11)

include(FetchContent)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24")
    # Affects robustness of timestamp checking on FetchContent dependencies.
    cmake_policy(SET CMP0135 NEW)
endif ()


FetchContent_Declare(certify
        GIT_REPOSITORY https://github.com/cwaldren-ld/certify.git
        GIT_TAG 8fc3024f97ffcc7442e30e1fe6ddff54b45e35d6
)

# The tests in certify don't compile.
set(PREVIOUS_BUILD_TESTING ${BUILD_TESTING})
set(BUILD_TESTING OFF)

FetchContent_MakeAvailable(certify)

set(BUILD_TESTING ${PREVIOUS_BUILD_TESTING})

install(
        TARGETS core
        EXPORT "${PROJECT_NAME}Targets"
)
