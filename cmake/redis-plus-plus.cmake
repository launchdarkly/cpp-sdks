cmake_minimum_required(VERSION 3.11)

include(FetchContent)


FetchContent_Declare(hiredis
        GIT_REPOSITORY https://github.com/redis/hiredis.git
        # 1.3.0
        GIT_TAG ccad7ebaf99310957004661d1c5f82d2a33ebd10
        GIT_SHALLOW TRUE
        SOURCE_DIR _deps/hiredis
        OVERRIDE_FIND_PACKAGE
)


FetchContent_MakeAvailable(hiredis)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/_deps)

set(REDIS_PLUS_PLUS_BUILD_TEST OFF CACHE BOOL "" FORCE)

# 1.3.7 is the last release that works with FetchContent, due to a problem with CheckSymbolExists
# when it tries to do feature detection on hiredis.
FetchContent_Declare(redis-plus-plus
        GIT_REPOSITORY https://github.com/sewenew/redis-plus-plus.git
        # 1.3.15
        GIT_TAG a63ac43bf192772910b52e27cd2b42a6098a0071
        GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(redis-plus-plus)
