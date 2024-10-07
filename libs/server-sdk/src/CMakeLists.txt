
file(GLOB HEADER_LIST CONFIGURE_DEPENDS
        "${LaunchDarklyCPPServer_SOURCE_DIR}/include/launchdarkly/server_side/*.hpp"
        "${LaunchDarklyCPPServer_SOURCE_DIR}/include/launchdarkly/server_side/integrations/*.hpp"
)

if (LD_BUILD_SHARED_LIBS)
    message(STATUS "LaunchDarkly: building server-sdk as shared library")
    add_library(${LIBNAME} SHARED)
else ()
    message(STATUS "LaunchDarkly: building server-sdk as static library")
    add_library(${LIBNAME} STATIC)
endif ()


target_sources(${LIBNAME}
        PRIVATE
        ${HEADER_LIST}
        client.cpp
        client_impl.cpp
        data_source_status.cpp
        config/config.cpp
        config/config_builder.cpp
        config/builders/data_system/background_sync_builder.cpp
        config/builders/data_system/bootstrap_builder.cpp
        config/builders/data_system/data_system_builder.cpp
        config/builders/data_system/lazy_load_builder.cpp
        config/builders/data_system/data_destination_builder.cpp
        all_flags_state/all_flags_state.cpp
        all_flags_state/json_all_flags_state.cpp
        all_flags_state/all_flags_state_builder.cpp
        integrations/data_reader/kinds.cpp
        data_components/change_notifier/change_notifier.hpp
        data_components/change_notifier/change_notifier.cpp
        data_components/dependency_tracker/dependency_tracker.hpp
        data_components/dependency_tracker/dependency_tracker.cpp
        data_components/expiration_tracker/expiration_tracker.hpp
        data_components/expiration_tracker/expiration_tracker.cpp
        data_components/memory_store/memory_store.hpp
        data_components/memory_store/memory_store.cpp
        data_components/serialization_adapters/json_deserializer.hpp
        data_components/serialization_adapters/json_deserializer.cpp
        data_components/serialization_adapters/json_destination.hpp
        data_components/serialization_adapters/json_destination.cpp
        data_systems/background_sync/detail/payload_filter_validation/payload_filter_validation.hpp
        data_systems/background_sync/detail/payload_filter_validation/payload_filter_validation.cpp
        data_systems/background_sync/sources/polling/polling_data_source.hpp
        data_systems/background_sync/sources/polling/polling_data_source.cpp
        data_systems/background_sync/sources/streaming/streaming_data_source.hpp
        data_systems/background_sync/sources/streaming/streaming_data_source.cpp
        data_systems/background_sync/sources/streaming/event_handler.hpp
        data_systems/background_sync/sources/streaming/event_handler.cpp
        data_systems/background_sync/background_sync_system.hpp
        data_systems/background_sync/background_sync_system.cpp
        data_systems/lazy_load/lazy_load_system.hpp
        data_systems/lazy_load/lazy_load_system.cpp
        data_systems/offline.hpp
        data_systems/offline.cpp
        data_interfaces.cpp
        evaluation/evaluator.cpp
        evaluation/rules.cpp
        evaluation/bucketing.cpp
        evaluation/operators.cpp
        evaluation/evaluation_error.cpp
        evaluation/evaluation_stack.cpp
        evaluation/detail/semver_operations.cpp
        evaluation/detail/timestamp_operations.cpp
        events/event_factory.cpp
        bindings/c/sdk.cpp
        bindings/c/builder.cpp
        bindings/c/config.cpp
        bindings/c/all_flags_state/all_flags_state.cpp
)


target_link_libraries(${LIBNAME}
        PUBLIC launchdarkly::common
        PRIVATE Boost::headers Boost::json Boost::url launchdarkly::sse launchdarkly::internal foxy timestamp)


add_library(launchdarkly::server ALIAS ${LIBNAME})

if (LD_BUILD_SHARED_LIBS AND MSVC)
    install(FILES $<TARGET_PDB_FILE:${LIBNAME}> DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
endif ()

# Using PUBLIC_HEADERS would flatten the include.
# This will preserve it, but dependencies must do the same.
install(DIRECTORY "${LaunchDarklyCPPServer_SOURCE_DIR}/include/launchdarkly"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Need the public headers to build.
target_include_directories(${LIBNAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
)

# Minimum C++ standard needed for consuming the public API is C++17.
target_compile_features(${LIBNAME} PUBLIC cxx_std_17)

install(
        TARGETS ${LIBNAME} OPTIONAL
        EXPORT "${PROJECT_NAME}Targets"

)
