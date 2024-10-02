file(GLOB HEADER_LIST CONFIGURE_DEPENDS
        "${LaunchDarklyCPPServerRedisSource_SOURCE_DIR}/include/launchdarkly/server_side/integrations/redis/*.hpp"
)

if (LD_BUILD_SHARED_LIBS)
    message(STATUS "LaunchDarkly: building server-sdk-redis-source as shared library")
    add_library(${LIBNAME} SHARED)
else ()
    message(STATUS "LaunchDarkly: building server-sdk-redis-source as static library")
    add_library(${LIBNAME} STATIC)
endif ()

add_library(launchdarkly::server_redis_source ALIAS ${LIBNAME})


target_sources(${LIBNAME}
        PRIVATE
        ${HEADER_LIST}
        redis_source.cpp
        bindings/redis/redis_source.cpp
)

target_link_libraries(${LIBNAME}
        PUBLIC launchdarkly::server
        PRIVATE
        redis++::redis++_static
)

# Optional in case only the client SDK is being built.
install(TARGETS ${LIBNAME} OPTIONAL)
if (LD_BUILD_SHARED_LIBS AND MSVC)
    install(FILES $<TARGET_PDB_FILE:${LIBNAME}> DESTINATION bin OPTIONAL)
endif ()

# Using PUBLIC_HEADERS would flatten the include.
# This will preserve it, but dependencies must do the same.

install(DIRECTORY "${LaunchDarklyCPPServerRedisSource_SOURCE_DIR}/include/launchdarkly"
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
        EXPORT launchdarklyTargets
)
