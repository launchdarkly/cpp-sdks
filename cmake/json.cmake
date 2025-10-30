cmake_minimum_required(VERSION 3.11)

include(FetchContent)

set(JSON_ImplicitConversions OFF)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24")
    # Affects robustness of timestamp checking on FetchContent dependencies.
    cmake_policy(SET CMP0135 NEW)
endif ()

# Use the same FetchContent name as OpenTelemetry to avoid duplicate targets
FetchContent_Declare(nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)

FetchContent_MakeAvailable(nlohmann_json)
