
file(GLOB HEADER_LIST CONFIGURE_DEPENDS
        "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/*.hpp"
        "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/c_bindings/*.hpp"
        "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/config/*.hpp"
        "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/config/detail/*.hpp"
        "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/data/*.hpp"
        "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/events/*.hpp"
        "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/network/*.hpp"
        "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/network/detail/*.hpp"
        "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/serialization/*.hpp"
        "${LaunchDarklyCommonSdk_SOURCE_DIR}/include/serialization/events/*.hpp"
        )

# Automatic library: static or dynamic based on user config.
add_library(${LIBNAME}
        ${HEADER_LIST}
        logger.cpp
        console_backend.cpp
        log_level.cpp
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
        context_filter.cpp
        config/app_info_builder.cpp
        value_mapping.cpp
        serialization/json_attributes.cpp
        serialization/json_context.cpp
        serialization/json_evaluation_reason.cpp
        serialization/json_evaluation_result.cpp
        serialization/json_value.cpp
        serialization/events/json_events.cpp
        serialization/json_errors.cpp
        events/asio_event_processor.cpp
        events/outbox.cpp
        events/worker_pool.cpp
        events/summarizer.cpp
        events/client_events.cpp
        events/request_worker.cpp
        events/event_batch.cpp
        config/http_properties.cpp
        config/data_source_builder.cpp
        config/http_properties_builder.cpp
        network/http_requester.cpp
        network/http_error_messages.cpp
        c_bindings/value.cpp
        c_bindings/array_builder.cpp
        c_bindings/object_builder.cpp
        c_bindings/context_builder.cpp
        c_bindings/status.cpp
        c_bindings/context.cpp
        c_bindings/config/builder.cpp
        c_bindings/config/config.cpp
        )


add_library(launchdarkly::common ALIAS ${LIBNAME})

install(TARGETS ${LIBNAME})

# Using PUBLIC_HEADERS would flatten the include.
# This will preserve it, but dependencies must do the same.
install(DIRECTORY "${LaunchDarklyCommonSdk_SOURCE_DIR}/include"
        DESTINATION "include"
        )

message(STATUS "LaunchDarklyCommonSdk_SOURCE_DIR=${LaunchDarklyCommonSdk_SOURCE_DIR}")

target_link_libraries(${LIBNAME}
        PUBLIC Boost::headers tl::expected foxy
        PRIVATE Boost::url Boost::json OpenSSL::SSL Boost::disable_autolinking)

# Need the public headers to build.
target_include_directories(${LIBNAME} PUBLIC ../include)

# Minimum C++ standard needed for consuming the public API is C++17.
target_compile_features(${LIBNAME} PUBLIC cxx_std_17)
