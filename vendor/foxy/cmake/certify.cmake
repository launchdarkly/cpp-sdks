cmake_minimum_required(VERSION 3.11)

include(FetchContent)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24")
    # Affects robustness of timestamp checking on FetchContent dependencies.
    cmake_policy(SET CMP0135 NEW)
endif ()


FetchContent_Declare(certify
        GIT_REPOSITORY https://github.com/jens-diewald/certify.git
        GIT_TAG 9185a824e2085b5632be542c0377204a05a4fa40
)

# The tests in certify don't compile.
set(PREVIOUS_BUILD_TESTING ${BUILD_TESTING})
set(BUILD_TESTING OFF)

FetchContent_MakeAvailable(certify)

set(BUILD_TESTING ${PREVIOUS_BUILD_TESTING})

# Override the include directories for certify::core
target_include_directories(core INTERFACE
        $<BUILD_INTERFACE:${certify_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
