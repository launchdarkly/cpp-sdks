cmake_minimum_required(VERSION 3.11)

include(FetchContent)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24")
    # Affects robustness of timestamp checking on FetchContent dependencies.
    cmake_policy(SET CMP0135 NEW)
endif ()

if (POLICY CMP0169)
    # TODO: Use FetchContent MakeAvailable. I believe this wasn't used because EXCLUDE_FROM_ALL needs to be passed (?).
    cmake_policy(SET CMP0169 OLD)
endif ()

if (POLICY CMP0167)
    # TODO: Update to use the Boost project's cmake config directly, since FindBoost was deprecated in
    # cmake >= 3.30.
    cmake_policy(SET CMP0167 OLD)
endif ()


FetchContent_Declare(boost_certify
        GIT_REPOSITORY https://github.com/djarek/certify.git
        GIT_TAG 97f5eebfd99a5d6e99d07e4820240994e4e59787
)

set(BUILD_TESTING OFF)

FetchContent_GetProperties(boost_certify)
if (NOT boost_certify_POPULATED)
    FetchContent_Populate(boost_certify)
    # EXCLUDE_FROM_ALL is specified here because the installation of certify is broken.
    add_subdirectory(${boost_certify_SOURCE_DIR} ${boost_certify_BINARY_DIR} EXCLUDE_FROM_ALL)
endif ()

set(BUILD_TESTING "${ORIGINAL_BUILD_TESTING}")
