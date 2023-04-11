
file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${LaunchDarklyCPPClient_SOURCE_DIR}/include/launchdarkly/*.hpp")

# Automatic library: static or dynamic based on user config.
add_library(${LIBNAME} api.cpp ${HEADER_LIST} data_sources/streaming_data_source.cpp data_sources/base_64.cpp data_sources/streaming_data_handler.cpp data_source_update_sink.cpp)

target_link_libraries(${LIBNAME} launchdarkly::common launchdarkly::sse)

add_library(launchdarkly::client ALIAS ${LIBNAME})

# Need the public headers to build.
target_include_directories(${LIBNAME} PUBLIC ../include)

# Minimum C++ standard needed for consuming the public API is C++17.
target_compile_features(${LIBNAME} PUBLIC cxx_std_17)
