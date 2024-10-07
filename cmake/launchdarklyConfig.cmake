include(CMakeFindDependencyMacro)

if (NOT DEFINED Boost_USE_STATIC_LIBS)
    if (LD_DYNAMIC_LINK_BOOST)
        set(Boost_USE_STATIC_LIBS OFF)
    else ()
        set(Boost_USE_STATIC_LIBS ON)
    endif ()
endif ()

find_dependency(Boost 1.81 COMPONENTS json url coroutine)
find_dependency(OpenSSL)
find_dependency(tl-expected)
find_dependency(certify)

include(${CMAKE_CURRENT_LIST_DIR}/launchdarklyTargets.cmake)
