
file(GLOB HEADER_LIST CONFIGURE_DEPENDS
        "${LaunchDarklyCPPServer_SOURCE_DIR}/include/launchdarkly/server_side/*.hpp"
        )

# Automatic library: static or dynamic based on user config.

add_library(${LIBNAME}
        ${HEADER_LIST}
        boost.cpp
        evaluation/detail/evaluation_stack.cpp
        evaluation/evaluator.cpp
        evaluation/rules.cpp
        evaluation/bucketing.cpp
        evaluation/operators.cpp
        evaluation/evaluation_error.cpp
        evaluation/detail/timestamp_operations.cpp
        evaluation/detail/semver_operations.cpp
        flag_manager/flag_store.cpp
        )

if (MSVC OR (NOT BUILD_SHARED_LIBS))
    target_link_libraries(${LIBNAME}
            PUBLIC launchdarkly::common
            PRIVATE Boost::headers Boost::json Boost::url launchdarkly::sse launchdarkly::internal foxy timestamp)
else ()
    # The default static lib builds, for linux, are positition independent.
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