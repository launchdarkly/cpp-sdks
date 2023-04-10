
file(GLOB HEADER_LIST CONFIGURE_DEPENDS
        "${LaunchDarklyCPPCommon_SOURCE_DIR}/include/*.hpp"
        "${LaunchDarklyCPPCommon_SOURCE_DIR}/include/config/*.hpp"
        "${LaunchDarklyCPPCommon_SOURCE_DIR}/include/events/*.hpp"
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
        config/service_hosts.cpp
        config/events.cpp
        config/hosts_builder.cpp
        config/events_builder.cpp
        config/config_builder.cpp
        config/config.cpp
        data/evaluation_reason.cpp
        data/evaluation_detail_internal.cpp
        data/evaluation_result.cpp
        context_filter.cpp
        config/application_info.cpp
        lib.cpp
        value_mapping.cpp
        serialization/json_attributes.cpp
        serialization/json_context.cpp
        serialization/json_evaluation_reason.cpp
        serialization/json_evaluation_result.cpp
        serialization/json_value.cpp
        serialization/events/json_events.cpp
        events/null_event_processor.cpp
        events/asio_event_processor.cpp
        events/outbox.cpp
        events/conn_pool.cpp
        events/summary_state.cpp
        )

add_library(launchdarkly::common ALIAS ${LIBNAME})

target_link_libraries(${LIBNAME} Boost::headers tl::expected)

# Need the public headers to build.
target_include_directories(${LIBNAME} PUBLIC ../include)

# Minimum C++ standard needed for consuming the public API is C++17.
target_compile_features(${LIBNAME} PUBLIC cxx_std_17)
