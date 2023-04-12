
file(GLOB HEADER_LIST CONFIGURE_DEPENDS
        "${LaunchDarklySSEClient_SOURCE_DIR}/include/launchdarkly/*.hpp")

# Automatic library: static or dynamic based on user config.
add_library(${LIBNAME} client.cpp parser.cpp event.cpp boost-url.cpp ${HEADER_LIST} backoff.cpp)
target_link_libraries(${LIBNAME}
        PUBLIC OpenSSL::SSL Boost::headers
        PRIVATE certify::core
        )

add_library(launchdarkly::sse ALIAS ${LIBNAME})

# Need the public headers to build.
target_include_directories(${LIBNAME} PUBLIC ../include)

# Minimum C++ standard needed for consuming the public API is C++17.
target_compile_features(${LIBNAME} PUBLIC cxx_std_17)
