cmake_minimum_required(VERSION 3.11)

include(FetchContent)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24")
    # Affects robustness of timestamp checking on FetchContent dependencies.
    cmake_policy(SET CMP0135 NEW)
endif ()

FetchContent_Declare(boost_certify
        GIT_REPOSITORY https://github.com/djarek/certify.git
        GIT_TAG 97f5eebfd99a5d6e99d07e4820240994e4e59787
        )

set(BUILD_TESTING OFF)

FetchContent_MakeAvailable(boost_certify)

set(BUILD_TESTING "${ORIGINAL_BUILD_TESTING}")

set(BUILD_SHARED_LIBS "${ORIGINAL_BUILD_SHARED_LIBS}")