cmake_minimum_required(VERSION 3.11)

include(FetchContent)

FetchContent_Declare(foxy
        GIT_REPOSITORY https://github.com/launchdarkly/foxy.git
        GIT_TAG cw/add-perpetual-request
        )

set(BUILD_TESTING OFF)
FetchContent_MakeAvailable(foxy)
