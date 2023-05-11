
file(GLOB HEADER_LIST CONFIGURE_DEPENDS
        "${LaunchDarklyCPPClient_SOURCE_DIR}/include/launchdarkly/*.hpp"
        "${LaunchDarklyCPPClient_SOURCE_DIR}/include/launchdarkly/client_side/*.hpp"
        "${LaunchDarklyCPPClient_SOURCE_DIR}/include/launchdarkly/client_side/data_sources/*.hpp"
        "${LaunchDarklyCPPClient_SOURCE_DIR}/include/launchdarkly/client_side/data_sources/detail/*.hpp"
        "${LaunchDarklyCPPClient_SOURCE_DIR}/include/launchdarkly/client_side/flag_manager/*.hpp"
        "${LaunchDarklyCPPClient_SOURCE_DIR}/include/launchdarkly/client_side/flag_manager/detail/*.hpp"
        "${LaunchDarklyCPPClient_SOURCE_DIR}/include/launchdarkly/client_side/event_processor/*.hpp"
        "${LaunchDarklyCPPClient_SOURCE_DIR}/include/launchdarkly/client_side/event_processor/detail/*.hpp"
        )

# Automatic library: static or dynamic based on user config.

add_library(${LIBNAME}
        ${HEADER_LIST}
        data_sources/streaming_data_source.cpp
        data_sources/base_64.cpp
        data_sources/data_source_event_handler.cpp
        data_source_update_sink.cpp
        data_sources/polling_data_source.cpp
        flag_manager/flag_manager.cpp
        flag_manager/flag_updater.cpp
        flag_manager/flag_change_event.cpp
        data_sources/data_source_status.cpp
        data_sources/data_source_status_manager.cpp
        event_processor/event_processor.cpp
        event_processor/null_event_processor.cpp
        boost_signal_connection.cpp
        client_impl.cpp
        client.cpp)


target_link_libraries(${LIBNAME}
        PUBLIC launchdarkly::common
        PRIVATE launchdarkly::sse)

add_library(launchdarkly::client ALIAS ${LIBNAME})

install(TARGETS ${LIBNAME})
# Using PUBLIC_HEADERS would flatten the include.
# This will preserve it, but dependencies must do the same.

install(DIRECTORY "${LaunchDarklyCPPClient_SOURCE_DIR}/include/launchdarkly"
        DESTINATION "include"
        )

# Need the public headers to build.
target_include_directories(${LIBNAME} PUBLIC ../include)

# Minimum C++ standard needed for consuming the public API is C++17.
target_compile_features(${LIBNAME} PUBLIC cxx_std_17)
