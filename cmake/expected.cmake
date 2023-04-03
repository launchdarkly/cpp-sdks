cmake_minimum_required(VERSION 3.11)

include(FetchContent)

FetchContent_Declare(tl-expected
        GIT_REPOSITORY https://github.com/TartanLlama/expected.git
        GIT_TAG 292eff8bd8ee230a7df1d6a1c00c4ea0eb2f0362
        )

FetchContent_MakeAvailable(tl-expected)
