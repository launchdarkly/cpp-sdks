
file(GLOB HEADER_LIST CONFIGURE_DEPENDS
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/*.hpp"
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/bindings/c/*.hpp"
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/config/*.hpp"
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/config/detail/*.hpp"
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/data/*.hpp"
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/events/*.hpp"
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/network/*.hpp"
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/network/detail/*.hpp"
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/serialization/*.hpp"
        "${LaunchDarklyInternalSdk_SOURCE_DIR}/include/serialization/events/*.hpp"
        )

# Automatic library: static or dynamic based on user config.
add_library(${LIBNAME}
        ${HEADER_LIST}
        context_filter.cpp
        events/asio_event_processor.cpp
        events/client_events.cpp
        events/event_batch.cpp
        events/outbox.cpp
        events/request_worker.cpp
        events/summarizer.cpp
        events/worker_pool.cpp
        logging/console_backend.cpp
        logging/console_backend.hpp
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
        )


add_library(launchdarkly::internal ALIAS ${LIBNAME})

install(TARGETS ${LIBNAME})

# Using PUBLIC_HEADERS would flatten the include.
# This will preserve it, but dependencies must do the same.
install(DIRECTORY "${LaunchDarklyInternalSdk_SOURCE_DIR}/include"
        DESTINATION "include"
        )

message(STATUS "LaunchDarklyInternalSdk_SOURCE_DIR=${LaunchDarklyInternalSdk_SOURCE_DIR}")

target_link_libraries(${LIBNAME}
        PUBLIC Boost::headers tl::expected foxy
        PRIVATE Boost::url Boost::json OpenSSL::SSL Boost::disable_autolinking)

# Need the public headers to build.
target_include_directories(${LIBNAME} PUBLIC ../include)

# Minimum C++ standard needed for consuming the public API is C++17.
target_compile_features(${LIBNAME} PUBLIC cxx_std_17)
