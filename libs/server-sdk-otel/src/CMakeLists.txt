file(GLOB HEADER_LIST CONFIGURE_DEPENDS
        "${LaunchDarklyCPPServerOtel_SOURCE_DIR}/include/launchdarkly/server_side/integrations/otel/*.hpp"
)

if (LD_BUILD_SHARED_LIBS)
    message(STATUS "LaunchDarkly: building server-sdk-otel as shared library")
    add_library(${LIBNAME} SHARED)
else ()
    message(STATUS "LaunchDarkly: building server-sdk-otel as static library")
    add_library(${LIBNAME} STATIC)
endif ()

target_sources(${LIBNAME}
        PRIVATE
        ${HEADER_LIST}
        tracing_hook.cpp
)

target_link_libraries(${LIBNAME}
        PUBLIC launchdarkly::server
        PUBLIC opentelemetry-cpp::api
)

add_library(launchdarkly::server_otel ALIAS ${LIBNAME})

if (LD_BUILD_SHARED_LIBS AND MSVC)
    install(FILES $<TARGET_PDB_FILE:${LIBNAME}> DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
endif ()

# Using PUBLIC_HEADERS would flatten the include.
# This will preserve it, but dependencies must do the same.

install(DIRECTORY "${LaunchDarklyCPPServerOtel_SOURCE_DIR}/include/launchdarkly"
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

# Note: We don't export this target because OpenTelemetry is a peer dependency
# and consumers need to link against it directly in their own projects
install(
        TARGETS ${LIBNAME} OPTIONAL
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
