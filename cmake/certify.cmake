cmake_minimum_required(VERSION 3.11)

include(FetchContent)

FetchContent_Declare(certify
        GIT_REPOSITORY https://github.com/djarek/certify.git
        GIT_TAG        97f5eebfd99a5d6e99d07e4820240994e4e59787
)

FetchContent_MakeAvailable(certify)
