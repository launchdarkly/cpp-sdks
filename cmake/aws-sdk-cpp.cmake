cmake_minimum_required(VERSION 3.11)

include(FetchContent)

# Limit the AWS SDK build to just the DynamoDB service client.
# Without this, the SDK builds ~50 service clients and CI time becomes untenable.
set(BUILD_ONLY "dynamodb" CACHE STRING "" FORCE)

# We don't want to build or run the AWS SDK's own tests.
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(AUTORUN_UNIT_TESTS OFF CACHE BOOL "" FORCE)

# Don't install the AWS SDK headers/libs alongside our own.
set(AWS_SDK_WARNINGS_ARE_ERRORS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(aws-sdk-cpp
        GIT_REPOSITORY https://github.com/aws/aws-sdk-cpp.git
        # 1.11.805
        GIT_TAG ecae762dfdddf9b538c1b490007f4dd5aa16a9bb
        # aws-sdk-cpp uses git submodules for aws-crt-cpp and the underlying
        # AWS C SDK libraries; FetchContent must clone them recursively.
        GIT_SUBMODULES_RECURSE TRUE
        OVERRIDE_FIND_PACKAGE
)

# Always build the AWS SDK as static archives, even when our own library is
# being built shared (LD_BUILD_SHARED_LIBS=ON). Building aws-sdk-cpp as
# BUILD_SHARED_LIBS=ON produces dylibs on macOS whose dynamodb component
# fails to find AWS Core symbols at link time (the AWS SDK's visibility
# configuration doesn't export them consistently across its FetchContent
# build). Linking aws-sdk-cpp statically into our shared wrapper sidesteps
# the issue.
#
# This needs cache manipulation rather than a function-scoped `set()`:
# aws-sdk-cpp's top-level CMakeLists pins `cmake_policy(SET CMP0077 OLD)`,
# which makes its `option(BUILD_SHARED_LIBS ... ON)` ignore non-cache
# variables and unconditionally write the cache. The only way to stop
# `option()` from setting the cache to ON is to pre-populate the cache
# with OFF before FetchContent_MakeAvailable runs.
#
# The prior cache state is saved before the FORCE-write and restored
# immediately after FetchContent finishes, so other subprojects (e.g.
# libs/server-sdk-otel) see whatever BUILD_SHARED_LIBS value was in the
# cache when this file was first included. An earlier attempt that
# FORCE-wrote OFF without restoring leaked into those subprojects; the
# save/restore here is the fix for that.
if (DEFINED CACHE{BUILD_SHARED_LIBS})
    set(_LD_AWS_BSL_PREV "${BUILD_SHARED_LIBS}")
    set(_LD_AWS_BSL_WAS_CACHED TRUE)
else ()
    set(_LD_AWS_BSL_WAS_CACHED FALSE)
endif ()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(aws-sdk-cpp)

if (_LD_AWS_BSL_WAS_CACHED)
    set(BUILD_SHARED_LIBS "${_LD_AWS_BSL_PREV}" CACHE BOOL "" FORCE)
else ()
    unset(BUILD_SHARED_LIBS CACHE)
endif ()
unset(_LD_AWS_BSL_PREV)
unset(_LD_AWS_BSL_WAS_CACHED)
