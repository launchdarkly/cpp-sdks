
file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${LaunchDarklyCPPClient_SOURCE_DIR}/include/launchdarkly/*.hpp")

# Automatic library: static or dynamic based on user config.
add_library(${LIBNAME} api.cpp ${HEADER_LIST})

add_library(launchdarkly::client ALIAS ${LIBNAME})

# Need the public headers to build.
target_include_directories(${LIBNAME} PUBLIC ../include)

# Minimum C++ standard needed for consuming the public API is C++17.
target_compile_features(${LIBNAME} PUBLIC cxx_std_17)

# For the IDEs.
source_group(
	TREE "${PROJECT_SOURCE_DIR}/include"
	PREFIX "C++ Headers"
	FILES ${HEADER_LIST}
)
