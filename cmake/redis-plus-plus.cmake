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

FetchContent_Declare(redis-plus-plus
        GIT_REPOSITORY https://github.com/sewenew/redis-plus-plus.git
        GIT_TAG 8b9ce389099608cf9bae617d79d257d2cc05e12f
        GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(redis-plus-plus)
