
file(GLOB HEADER_LIST CONFIGURE_DEPENDS
        "${LaunchDarklySSEClient_SOURCE_DIR}/include/launchdarkly/*.hpp"
        "${LaunchDarklySSEClient_SOURCE_DIR}/include/launchdarkly/sse/*.hpp"
)

# Automatic library: static or dynamic based on user config.
add_library(${LIBNAME} OBJECT
        ${HEADER_LIST}
        client.cpp
        parser.cpp
        event.cpp
        error.cpp
        backoff.cpp)

target_link_libraries(${LIBNAME}
        PUBLIC OpenSSL::SSL Boost::headers foxy
        PRIVATE Boost::url Boost::disable_autolinking
)

add_library(launchdarkly::sse ALIAS ${LIBNAME})

install(TARGETS ${LIBNAME})

# Need the public headers to build.
target_include_directories(${LIBNAME} PUBLIC ../include)

# Minimum C++ standard needed for consuming the public API is C++17.
target_compile_features(${LIBNAME} PUBLIC cxx_std_17)
