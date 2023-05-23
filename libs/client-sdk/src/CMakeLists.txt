
file(GLOB HEADER_LIST CONFIGURE_DEPENDS
        "${LaunchDarklyCPPClient_SOURCE_DIR}/include/launchdarkly/client_side/*.hpp"
        )

# Automatic library: static or dynamic based on user config.

add_library(${LIBNAME}
        ${HEADER_LIST}
        data_sources/streaming_data_source.cpp
        data_sources/data_source_event_handler.cpp
        data_sources/data_source_update_sink.cpp
        data_sources/polling_data_source.cpp
        flag_manager/flag_store.cpp
        flag_manager/flag_updater.cpp
        flag_manager/flag_change_event.cpp
        data_sources/data_source_status.cpp
        data_sources/data_source_status_manager.cpp
        event_processor/event_processor.cpp
        event_processor/null_event_processor.cpp
        boost_signal_connection.cpp
        client_impl.cpp
        client.cpp
        boost_signal_connection.hpp
        client_impl.hpp
        data_sources/data_source.hpp
        data_sources/data_source_event_handler.hpp
        data_sources/data_source_status_manager.hpp
        data_sources/data_source_update_sink.hpp
        data_sources/polling_data_source.hpp
        data_sources/streaming_data_source.hpp
        event_processor/event_processor.hpp
        event_processor/null_event_processor.hpp
        flag_manager/flag_store.hpp
        flag_manager/flag_updater.hpp
        event_processor.hpp
        bindings/c/sdk.cpp
        data_sources/null_data_source.cpp
        flag_manager/context_index.cpp
        serialization/json_all_flags.hpp
        serialization/json_all_flags.cpp
        flag_manager/flag_manager.cpp
        flag_manager/flag_persistence.cpp
        bindings/c/sdk.cpp)

if(MSVC OR (NOT BUILD_SHARED_LIBS))
        target_link_libraries(${LIBNAME}
                PUBLIC launchdarkly::common
                PRIVATE Boost::headers Boost::json Boost::url launchdarkly::sse launchdarkly::internal foxy)
else()
        # The default static lib builds, for linux, are positition independent.
        # So they do not link into a shared object without issues. So, when
        # building shared objects do not link the static libraries and instead
        # use the "src.hpp" files for required libraries.
        # macOS shares the same path for simplicity.
        target_link_libraries(${LIBNAME}
        PUBLIC launchdarkly::common
        PRIVATE Boost::headers launchdarkly::sse launchdarkly::internal foxy)

        target_sources(${LIBNAME} PRIVATE boost.cpp)
endif ()

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
