include(CMakeFindDependencyMacro)

if (NOT DEFINED Boost_USE_STATIC_LIBS)
    if (LD_DYNAMIC_LINK_BOOST)
        set(Boost_USE_STATIC_LIBS OFF)
    else ()
        set(Boost_USE_STATIC_LIBS ON)
    endif ()
endif ()

# This policy is designed to force a choice between the old behavior of an integrated FindBoost implementation
# and the implementation provided in by boost.
if(POLICY CMP0167)
    # Uses the BoostConfig.cmake included with the boost distribution.
    cmake_policy(SET CMP0167 NEW)
endif()

find_dependency(Boost 1.81 COMPONENTS json url coroutine)
find_dependency(OpenSSL)
find_dependency(tl-expected)
find_dependency(certify)

# If the SDK was built with CURL networking support, CURL::libcurl will be
# referenced in the exported targets, so we need to find it.
# We use find_package directly with QUIET so it doesn't fail if CURL isn't needed.
find_package(CURL QUIET)

include(${CMAKE_CURRENT_LIST_DIR}/launchdarklyTargets.cmake)
