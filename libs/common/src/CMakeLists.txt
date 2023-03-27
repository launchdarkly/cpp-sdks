
file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${LaunchDarklyCPPCommon_SOURCE_DIR}/include/*.hpp")

# Automatic library: static or dynamic based on user config.
add_library(${LIBNAME} logger.cpp ${HEADER_LIST} console_backend.cpp log_level.cpp attribute_reference.cpp context.cpp context_builder.cpp attributes.cpp value.cpp)

add_library(launchdarkly::common ALIAS ${LIBNAME})

# Need the public headers to build.
target_include_directories(${LIBNAME} PUBLIC ../include)

# Minimum C++ standard needed for consuming the public API is C++17.
target_compile_features(${LIBNAME} PUBLIC cxx_std_17)
