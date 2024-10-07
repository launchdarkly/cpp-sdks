
file(GLOB HEADER_LIST CONFIGURE_DEPENDS
        "${LaunchDarklyCPPClient_SOURCE_DIR}/include/launchdarkly/client_side/*.hpp"
)

if (LD_BUILD_SHARED_LIBS)
    message(STATUS "LaunchDarkly: building client-sdk as shared library")
    add_library(${LIBNAME} SHARED)
else ()
    message(STATUS "LaunchDarkly: building client-sdk as static library")
    add_library(${LIBNAME} STATIC)
endif ()

target_sources(${LIBNAME} PRIVATE
        ${HEADER_LIST}
        data_sources/streaming_data_source.cpp
        data_sources/data_source_event_handler.cpp
        data_sources/polling_data_source.cpp
        flag_manager/flag_store.cpp
        flag_manager/flag_updater.cpp
        flag_manager/flag_change_event.cpp
        data_sources/data_source_status.cpp
        client_impl.cpp
        client.cpp
        client_impl.hpp
        data_sources/data_source_event_handler.hpp
        data_sources/data_source_status_manager.hpp
        data_sources/data_source_update_sink.hpp
        data_sources/polling_data_source.hpp
        data_sources/streaming_data_source.hpp
        flag_manager/flag_store.hpp
        flag_manager/flag_updater.hpp
        bindings/c/sdk.cpp
        bindings/c/builder.cpp
        bindings/c/config.cpp
        data_sources/null_data_source.cpp
        flag_manager/context_index.cpp
        flag_manager/flag_manager.cpp
        flag_manager/flag_persistence.cpp
)


target_link_libraries(${LIBNAME}
        PUBLIC launchdarkly::common
        PRIVATE Boost::headers Boost::json Boost::url launchdarkly::sse launchdarkly::internal foxy)


target_include_directories(${LIBNAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
)

# Minimum C++ standard needed for consuming the public API is C++17.
target_compile_features(${LIBNAME} PUBLIC cxx_std_17)

add_library(launchdarkly::client ALIAS ${LIBNAME})

if (LD_BUILD_SHARED_LIBS AND MSVC)
    install(FILES $<TARGET_PDB_FILE:${LIBNAME}> DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
endif ()

# Using PUBLIC_HEADERS would flatten the include.
# This will preserve it, but dependencies must do the same.

install(DIRECTORY "${LaunchDarklyCPPClient_SOURCE_DIR}/include/launchdarkly"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
        TARGETS ${LIBNAME} OPTIONAL
        EXPORT ${PROJECT_NAME}Targets
)
