include(CMakeFindDependencyMacro)

find_dependency(Boost 1.81 COMPONENTS json url coroutine)
find_dependency(OpenSSL)
find_dependency(tl-expected)
find_dependency(certify)

include(${CMAKE_CURRENT_LIST_DIR}/launchdarklyTargets.cmake)
