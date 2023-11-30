cmake_minimum_required(VERSION 3.11)

include(FetchContent)


FetchContent_Declare(hiredis
        GIT_REPOSITORY https://github.com/redis/hiredis.git
        GIT_TAG 60e5075d4ac77424809f855ba3e398df7aacefe8
        GIT_SHALLOW TRUE
        SOURCE_DIR _deps/hiredis
        OVERRIDE_FIND_PACKAGE
)

FetchContent_MakeAvailable(hiredis)

include_directories(${CMAKE_BINARY_DIR}/_deps)

set(REDIS_PLUS_PLUS_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(REDIS_PLUS_PLUS_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(REDIS_PLUS_PLUS_BUILD_STATIC ON CACHE BOOL "" FORCE)

# 1.3.7 is the last release that works with FetchContent, due to a problem with CheckSymbolExists
# when it tries to do feature detection on hiredis.
FetchContent_Declare(redis-plus-plus
        GIT_REPOSITORY https://github.com/sewenew/redis-plus-plus.git
        GIT_TAG 1.3.7
        GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(redis-plus-plus)
