cmake_minimum_required(VERSION 3.11)

include(FetchContent)

FetchContent_Declare(foxy
        GIT_REPOSITORY https://github.com/launchdarkly/foxy.git
        GIT_TAG 7f4ac0495ad2ed9cd0eca5994743d677ac1d2636
        )

set(BUILD_TESTING OFF)
FetchContent_MakeAvailable(foxy)
