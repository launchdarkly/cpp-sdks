file(GLOB HEADER_LIST CONFIGURE_DEPENDS
        "${LaunchDarklyCPPServerDynamoDBSource_SOURCE_DIR}/include/launchdarkly/server_side/integrations/dynamodb/*.hpp"
)

if (LD_BUILD_SHARED_LIBS)
    message(STATUS "LaunchDarkly: building server-sdk-dynamodb-source as shared library")
    add_library(${LIBNAME} SHARED)
else ()
    message(STATUS "LaunchDarkly: building server-sdk-dynamodb-source as static library")
    add_library(${LIBNAME} STATIC)
endif ()

target_sources(${LIBNAME}
        PRIVATE
        ${HEADER_LIST}
        dynamodb_source.cpp
)


target_link_libraries(${LIBNAME}
        PUBLIC launchdarkly::server
        PRIVATE
        aws-cpp-sdk-dynamodb
        aws-cpp-sdk-core
)


add_library(launchdarkly::server_dynamodb_source ALIAS ${LIBNAME})

if (LD_BUILD_SHARED_LIBS AND MSVC)
    install(FILES $<TARGET_PDB_FILE:${LIBNAME}> DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
endif ()

# Using PUBLIC_HEADERS would flatten the include.
# This will preserve it, but dependencies must do the same.

install(DIRECTORY "${LaunchDarklyCPPServerDynamoDBSource_SOURCE_DIR}/include/launchdarkly"
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
        EXPORT ${LD_TARGETS_EXPORT_NAME}
)
