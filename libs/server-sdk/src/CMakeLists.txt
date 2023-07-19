
file(GLOB HEADER_LIST CONFIGURE_DEPENDS
        "${LaunchDarklyCPPServer_SOURCE_DIR}/include/launchdarkly/server_side/*.hpp"
        "${LaunchDarklyCPPServer_SOURCE_DIR}/include/launchdarkly/server_side/integrations/*.hpp"
        )

# Automatic library: static or dynamic based on user config.

add_library(${LIBNAME}
        ${HEADER_LIST}
        boost.cpp
        client.cpp
        client_impl.cpp
        data_sources/data_source_update_sink.hpp
        data_store/data_store.hpp
        data_store/data_store_updater.hpp
        data_store/data_store_updater.cpp
        data_store/memory_store.cpp
        data_store/dependency_tracker.hpp
        data_store/dependency_tracker.cpp
        data_store/descriptors.hpp
        data_sources/data_source_event_handler.cpp
        data_sources/data_source_event_handler.hpp
        data_sources/data_source_status.cpp
        data_sources/polling_data_source.hpp
        data_sources/polling_data_source.cpp
        data_sources/data_source_status_manager.hpp
        data_sources/streaming_data_source.hpp
        data_sources/streaming_data_source.cpp
        data_sources/null_data_source.cpp
        evaluation/evaluator.cpp
        evaluation/rules.cpp
        evaluation/bucketing.cpp
        evaluation/operators.cpp
        evaluation/evaluation_error.cpp
        evaluation/detail/evaluation_stack.cpp
        evaluation/detail/semver_operations.cpp
        evaluation/detail/timestamp_operations.cpp
        )

if (MSVC OR (NOT BUILD_SHARED_LIBS))
    target_link_libraries(${LIBNAME}
            PUBLIC launchdarkly::common
            PRIVATE Boost::headers Boost::json Boost::url launchdarkly::sse launchdarkly::internal foxy timestamp)
else ()
    # The default static lib builds, for linux, are position independent.
    # So they do not link into a shared object without issues. So, when
    # building shared objects do not link the static libraries and instead
    # use the "src.hpp" files for required libraries.
    # macOS shares the same path for simplicity.
    target_link_libraries(${LIBNAME}
            PUBLIC launchdarkly::common
            PRIVATE Boost::headers launchdarkly::sse launchdarkly::internal foxy timestamp)

    target_sources(${LIBNAME} PRIVATE boost.cpp)
endif ()

add_library(launchdarkly::server ALIAS ${LIBNAME})

set_property(TARGET ${LIBNAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

install(TARGETS ${LIBNAME})
if (BUILD_SHARED_LIBS AND MSVC)
    install(FILES $<TARGET_PDB_FILE:${LIBNAME}> DESTINATION bin OPTIONAL)
endif ()
# Using PUBLIC_HEADERS would flatten the include.
# This will preserve it, but dependencies must do the same.

install(DIRECTORY "${LaunchDarklyCPPServer_SOURCE_DIR}/include/launchdarkly"
        DESTINATION "include"
        )

# Need the public headers to build.
target_include_directories(${LIBNAME} PUBLIC ../include)

# Minimum C++ standard needed for consuming the public API is C++17.
target_compile_features(${LIBNAME} PUBLIC cxx_std_17)
