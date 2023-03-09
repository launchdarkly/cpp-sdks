file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${LaunchDarklyCClient_SOURCE_DIR}/include/launchdarkly/*.h")

# Automatic library: static or dynamic based on user config.
add_library(${LIBNAME} api.cpp ${HEADER_LIST})

# Need the public C++ API headers to build, and also the C API headers.
target_include_directories(${LIBNAME} 
	PUBLIC 
		../include
)

target_link_libraries(${LIBNAME} launchdarkly::client)

# Needs C++ 17 internally to use the C++ API (only C is exposed, so no public requirement.)
target_compile_features(${LIBNAME} PRIVATE cxx_std_17)

# For the IDEs.
source_group(
        TREE "${PROJECT_SOURCE_DIR}/include/launchdarkly"
        PREFIX "C Headers"
        FILES ${HEADER_LIST}
)
