# This project aims to follow modern cmake guidelines, e.g.
# https://cliutils.gitlab.io/modern-cmake

# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyCPPServerOtel
        VERSION 0.1.0
        DESCRIPTION "LaunchDarkly C++ Server SDK OpenTelemetry Integration"
        LANGUAGES CXX
)

set(LIBNAME "launchdarkly-cpp-server-otel")

# If this project is the main CMake project (as opposed to being included via add_subdirectory)
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Disable C++ extensions for portability.
    set(CMAKE_CXX_EXTENSIONS OFF)
    # Enable folder support in IDEs.
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif ()

# Option for local development and CI builds
option(LD_BUILD_OTEL_FETCH_DEPS "Fetch OpenTelemetry dependencies for local/CI builds" OFF)

if (LD_BUILD_OTEL_FETCH_DEPS)
    # For local development and CI: fetch OpenTelemetry with sensible defaults
    message("LaunchDarkly: fetching OpenTelemetry dependencies for local/CI build")
    include(FetchContent)

    # Configure OpenTelemetry options with sensible defaults
    set(WITH_OTLP_HTTP ON CACHE BOOL "Build with OTLP HTTP exporter" FORCE)
    set(WITH_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)
    set(WITH_ABSEIL ON CACHE BOOL "Build with Abseil" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)

    FetchContent_Declare(
        opentelemetry-cpp
        GIT_REPOSITORY https://github.com/open-telemetry/opentelemetry-cpp.git
        GIT_TAG ea1f0d61ce5baa5584b097266bf133d1f31e3607  # v1.23.0
    )
    FetchContent_MakeAvailable(opentelemetry-cpp)
else()
    # Normal usage: find OpenTelemetry provided by the user
    # Users must provide OpenTelemetry themselves (via find_package or by setting paths)
    # This gives users full control over OpenTelemetry build configuration
    find_package(opentelemetry-cpp REQUIRED COMPONENTS api)
endif()

add_subdirectory(src)

if (LD_BUILD_UNIT_TESTS)
    add_subdirectory(tests)
endif ()
