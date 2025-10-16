
file(GLOB HEADER_LIST CONFIGURE_DEPENDS
        "${LaunchDarklyNetworking_SOURCE_DIR}/include/launchdarkly/network/*.hpp"
)

set(NETWORKING_SOURCES
        ${HEADER_LIST}
        curl_multi_manager.cpp
)

add_library(${LIBNAME} OBJECT ${NETWORKING_SOURCES})

add_library(launchdarkly::networking ALIAS ${LIBNAME})

message(STATUS "LaunchDarklyNetworking_SOURCE_DIR=${LaunchDarklyNetworking_SOURCE_DIR}")

target_link_libraries(${LIBNAME}
        PRIVATE
        Boost::headers
        CURL::libcurl
        Boost::disable_autolinking
)

# Need the public headers to build.
target_include_directories(${LIBNAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
)

# Minimum C++ standard needed for consuming the public API is C++17.
target_compile_features(${LIBNAME} PUBLIC cxx_std_17)

target_compile_definitions(${LIBNAME} PUBLIC LD_CURL_NETWORKING)

# Using PUBLIC_HEADERS would flatten the include.
# This will preserve it, but dependencies must do the same.
install(DIRECTORY "${LaunchDarklyNetworking_SOURCE_DIR}/include/launchdarkly"
        DESTINATION "include"
)

install(
        TARGETS ${LIBNAME}
        EXPORT ${LD_TARGETS_EXPORT_NAME}
)
