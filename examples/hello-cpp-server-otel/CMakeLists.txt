# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyHelloCPPServerOTel
        VERSION 0.1
        DESCRIPTION "LaunchDarkly Hello CPP Server-side SDK with OpenTelemetry Integration"
        LANGUAGES CXX
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(hello-cpp-server-otel main.cpp)

target_link_libraries(hello-cpp-server-otel
    PRIVATE
    launchdarkly::server
    launchdarkly::server_otel
    Threads::Threads
    opentelemetry_trace
    opentelemetry_exporter_otlp_http
)
