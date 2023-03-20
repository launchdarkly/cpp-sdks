cmake_minimum_required(VERSION 3.11)

include(FetchContent)

set(JSON_ImplicitConversions OFF)

FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)

FetchContent_MakeAvailable(json)
