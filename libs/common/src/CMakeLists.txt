
file(GLOB HEADER_LIST CONFIGURE_DEPENDS
        "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/*.hpp"
        "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/launchdarkly/*.hpp"
        "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/launchdarkly/bindings/c/*.h"
        "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/launchdarkly/bindings/c/config/*.h"
        "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/launchdarkly/bindings/c/data/*.h"
        "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/launchdarkly/config/client_side/*.hpp"
        "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/launchdarkly/config/shared/*.hpp"
        "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/launchdarkly/config/shared/builders/*.hpp"
        "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/launchdarkly/config/shared/built/*.hpp"
        "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/launchdarkly/data/*.hpp"
        "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/launchdarkly/logging/*.hpp"
)

# Automatic library: static or dynamic based on user config.
add_library(${LIBNAME} OBJECT
        ${HEADER_LIST}
        attribute_reference.cpp
        context.cpp
        context_builder.cpp
        attributes.cpp
        value.cpp
        attributes_builder.cpp
        error.cpp
        config/service_endpoints.cpp
        config/events.cpp
        config/endpoints_builder.cpp
        config/events_builder.cpp
        config/config_builder.cpp
        config/config.cpp
        data/evaluation_reason.cpp
        data/evaluation_detail_internal.cpp
        data/evaluation_detail.cpp
        data/evaluation_result.cpp
        config/app_info_builder.cpp
        config/http_properties.cpp
        config/data_source_builder.cpp
        config/http_properties_builder.cpp
        bindings/c/value.cpp
        bindings/c/array_builder.cpp
        bindings/c/object_builder.cpp
        bindings/c/context_builder.cpp
        bindings/c/status.cpp
        bindings/c/context.cpp
        bindings/c/config/logging_builder.cpp
        bindings/c/data/evaluation_detail.cpp
        bindings/c/listener_connection.cpp
        bindings/c/flag_listener.cpp
        bindings/c/memory_routines.cpp
        log_level.cpp
        config/persistence_builder.cpp
        config/logging_builder.cpp
)

add_library(launchdarkly::common ALIAS ${LIBNAME})

set_property(TARGET ${LIBNAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

install(TARGETS ${LIBNAME})

# Using PUBLIC_HEADERS would flatten the include.
# This will preserve it, but dependencies must do the same.
install(DIRECTORY "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/launchdarkly"
        DESTINATION "include"
)

message(STATUS "LaunchDarklyCommonSdk_SOURCE_DIR=${LaunchDarklyCommonSdk_SOURCE_DIR}")

target_link_libraries(${LIBNAME}
        PUBLIC tl::expected foxy
        PRIVATE
        Boost::headers
        Boost::url
        Boost::json
        OpenSSL::SSL
        Boost::disable_autolinking)

# Need the public headers to build.
target_include_directories(${LIBNAME} PUBLIC ../include)

# Minimum C++ standard needed for consuming the public API is C++17.
target_compile_features(${LIBNAME} PUBLIC cxx_std_17)
