cmake_minimum_required(VERSION 3.11)

include(FetchContent)


FetchContent_Declare(hiredis
        GIT_REPOSITORY https://github.com/redis/hiredis.git
        GIT_TAG 60e5075d4ac77424809f855ba3e398df7aacefe8
)

FetchContent_MakeAvailable(hiredis)

FetchContent_Declare(redis-plus-plus
        GIT_REPOSITORY https://github.com/sewenew/redis-plus-plus.git
        GIT_TAG 8b9ce389099608cf9bae617d79d257d2cc05e12f
)

FetchContent_MakeAvailable(redis-plus-plus)
