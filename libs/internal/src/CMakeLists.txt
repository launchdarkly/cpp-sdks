
file(GLOB HEADER_LIST CONFIGURE_DEPENDS
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/launchdarkly/*.hpp"
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/launchdarkly/events/*.hpp"
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/launchdarkly/network/*.hpp"
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/launchdarkly/serialization/*.hpp"
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/launchdarkly/serialization/events/*.hpp"
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/launchdarkly/signals/*.hpp"
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/launchdarkly/data_sources/*.hpp"
)

# Automatic library: static or dynamic based on user config.
set(INTERNAL_SOURCES
        ${HEADER_LIST}
        context_filter.cpp
        events/asio_event_processor.cpp
        events/null_event_processor.cpp
        events/common_events.cpp
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
        network/requester.cpp
        serialization/events/json_events.cpp
        serialization/json_attributes.cpp
        serialization/json_context.cpp
        serialization/json_evaluation_reason.cpp
        serialization/value_mapping.cpp
        serialization/json_evaluation_result.cpp
        serialization/json_sdk_data_set.cpp
        serialization/json_segment.cpp
        serialization/json_primitives.cpp
        serialization/json_rule_clause.cpp
        serialization/json_flag.cpp
        serialization/json_context_kind.cpp
        data_model/rule_clause.cpp
        data_model/flag.cpp
        encoding/base_64.cpp
        encoding/sha_256.cpp
        encoding/sha_1.cpp
        signals/boost_signal_connection.cpp)

if (LD_CURL_NETWORKING)
    message(STATUS "LaunchDarkly Internal: CURL networking enabled")
    find_package(CURL REQUIRED)
    list(APPEND INTERNAL_SOURCES network/curl_requester.cpp)
endif()

add_library(${LIBNAME} OBJECT ${INTERNAL_SOURCES})

add_library(launchdarkly::internal ALIAS ${LIBNAME})

# TODO(SC-209963): Remove once OpenSSL deprecated hash function usage has been updated
target_compile_options(${LIBNAME} PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wno-deprecated-declarations>
)

message(STATUS "LaunchDarklyInternalSdk_SOURCE_DIR=${LaunchDarklyInternalSdk_SOURCE_DIR}")

target_link_libraries(${LIBNAME}
        PUBLIC launchdarkly::common
        PRIVATE Boost::url Boost::json OpenSSL::SSL Boost::disable_autolinking Boost::headers tl::expected foxy)

if (LD_CURL_NETWORKING)
    target_link_libraries(${LIBNAME} PRIVATE CURL::libcurl)
    target_compile_definitions(${LIBNAME} PRIVATE LD_CURL_NETWORKING)
endif()

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
