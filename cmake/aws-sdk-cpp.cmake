cmake_minimum_required(VERSION 3.11)

include(FetchContent)

# Limit the AWS SDK build to just the DynamoDB service client.
# Without this, the SDK builds ~50 service clients and CI time becomes untenable.
set(BUILD_ONLY "dynamodb" CACHE STRING "" FORCE)

# We don't want to build or run the AWS SDK's own tests.
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(AUTORUN_UNIT_TESTS OFF CACHE BOOL "" FORCE)

# Don't install the AWS SDK headers/libs alongside our own.
set(AWS_SDK_WARNINGS_ARE_ERRORS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(aws-sdk-cpp
        GIT_REPOSITORY https://github.com/aws/aws-sdk-cpp.git
        # 1.11.805
        GIT_TAG ecae762dfdddf9b538c1b490007f4dd5aa16a9bb
        # aws-sdk-cpp uses git submodules for aws-crt-cpp and the underlying
        # AWS C SDK libraries; FetchContent must clone them recursively.
        GIT_SUBMODULES_RECURSE TRUE
        OVERRIDE_FIND_PACKAGE
)

FetchContent_MakeAvailable(aws-sdk-cpp)
