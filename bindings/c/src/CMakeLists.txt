file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${LaunchDarklyCClient_SOURCE_DIR}/include/launchdarkly/*.h")

# Automatic library: static or dynamic based on user config.
add_library(${LIBNAME} api.cpp ${HEADER_LIST})

add_library(launchdarkly::client-c ALIAS ${LIBNAME})

# Need the public C++ API headers to build, and also the C API headers.
target_include_directories(${LIBNAME}
        PUBLIC
        ../include
        ${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}
        )

target_link_libraries(${LIBNAME} launchdarkly::client)

# Needs C++ 17 internally to use the C++ API (only C is exposed, so no public requirement.)
target_compile_features(${LIBNAME} PRIVATE cxx_std_17)

set_target_properties(${LIBNAME} PROPERTIES
        POSITION_INDEPENDENT_CODE 1
        C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN 1
        SOVERSION ${PROJECT_VERSION_MAJOR}
        DEBUG_POSTFIX "_d"
        OUTPUT_NAME "api"
        PUBLIC_HEADER "api.h;${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/apiExport.h"
        MACOSX_RPATH ON
        )


target_compile_definitions(${LIBNAME}
        PUBLIC
        api_STATIC_DEFINE
        )
