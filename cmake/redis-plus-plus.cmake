cmake_minimum_required(VERSION 3.11)

include(FetchContent)

FetchContent_Declare(redis-plus-plus
        GIT_REPOSITORY https://github.com/sewenew/redis-plus-plus.git
        GIT_TAG 8b9ce389099608cf9bae617d79d257d2cc05e12f
)

FetchContent_MakeAvailable(redis-plus-plus)
