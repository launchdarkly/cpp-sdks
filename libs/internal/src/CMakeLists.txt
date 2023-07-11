
file(GLOB HEADER_LIST CONFIGURE_DEPENDS
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/launchdarkly/*.hpp"
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/launchdarkly/events/*.hpp"
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/launchdarkly/network/*.hpp"
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/launchdarkly/serialization/*.hpp"
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/launchdarkly/serialization/events/*.hpp"
        )

# Automatic library: static or dynamic based on user config.
add_library(${LIBNAME} OBJECT
        ${HEADER_LIST}
        context_filter.cpp
        events/asio_event_processor.cpp
        events/client_events.cpp
        events/event_batch.cpp
        events/outbox.cpp
        events/request_worker.cpp
        events/summarizer.cpp
        events/worker_pool.cpp
        events/lru_cache.cpp
        logging/console_backend.cpp
        logging/null_logger.cpp
        logging/logger.cpp
        network/http_error_messages.cpp
        network/http_requester.cpp
        serialization/events/json_events.cpp
        serialization/json_attributes.cpp
        serialization/json_context.cpp
        serialization/json_errors.cpp
        serialization/json_evaluation_reason.cpp
        serialization/json_value.cpp
        serialization/value_mapping.cpp
        serialization/json_evaluation_result.cpp
        serialization/json_sdk_data_set.cpp
        serialization/json_segment.cpp
        serialization/json_primitives.cpp
        serialization/json_rule_clause.cpp
        serialization/json_flag.cpp
        encoding/base_64.cpp
        encoding/sha_256.cpp)

add_library(launchdarkly::internal ALIAS ${LIBNAME})


set_property(TARGET ${LIBNAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

install(TARGETS ${LIBNAME})

message(STATUS "LaunchDarklyInternalSdk_SOURCE_DIR=${LaunchDarklyInternalSdk_SOURCE_DIR}")

target_link_libraries(${LIBNAME}
        PUBLIC launchdarkly::common
        PRIVATE Boost::url Boost::json OpenSSL::SSL Boost::disable_autolinking Boost::headers tl::expected foxy)

# Need the public headers to build.
target_include_directories(${LIBNAME} PUBLIC ../include)

# Minimum C++ standard needed for consuming the public API is C++17.
target_compile_features(${LIBNAME} PUBLIC cxx_std_17)
