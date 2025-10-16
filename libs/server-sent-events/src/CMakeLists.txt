
file(GLOB HEADER_LIST CONFIGURE_DEPENDS
        "${LaunchDarklySSEClient_SOURCE_DIR}/include/launchdarkly/*.hpp"
        "${LaunchDarklySSEClient_SOURCE_DIR}/include/launchdarkly/sse/*.hpp"
)

# Automatic library: static or dynamic based on user config.
set(SSE_SOURCES
        ${HEADER_LIST}
        client.cpp
        parser.cpp
        event.cpp
        error.cpp
        backoff_detail.cpp
        backoff.cpp
        curl_client.cpp
        backoff_timer.cpp)

if (LD_CURL_NETWORKING)
    message(STATUS "LaunchDarkly SSE: CURL networking enabled")
    find_package(CURL REQUIRED)
    list(APPEND SSE_SOURCES)
endif()

add_library(${LIBNAME} OBJECT ${SSE_SOURCES})

target_link_libraries(${LIBNAME}
        PUBLIC OpenSSL::SSL Boost::headers foxy
        PRIVATE Boost::url Boost::disable_autolinking
)

if (LD_CURL_NETWORKING)
    target_link_libraries(${LIBNAME} PRIVATE CURL::libcurl)
    target_compile_definitions(${LIBNAME} PRIVATE LD_CURL_NETWORKING)
endif()

add_library(launchdarkly::sse ALIAS ${LIBNAME})

# Need the public headers to build.
target_include_directories(${LIBNAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
)

# Minimum C++ standard needed for consuming the public API is C++17.
target_compile_features(${LIBNAME} PUBLIC cxx_std_17)


install(
        TARGETS ${LIBNAME}
        EXPORT ${LD_TARGETS_EXPORT_NAME}
)
