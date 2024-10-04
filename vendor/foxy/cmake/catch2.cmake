cmake_minimum_required(VERSION 3.11)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG fa43b77429ba76c462b1898d6cd2f2d7a9416b14
)

FetchContent_MakeAvailable(Catch2)
