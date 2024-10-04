
find_package(Boost 1.81 CONFIG REQUIRED COMPONENTS json url coroutine)
find_package(OpenSSL CONFIG REQUIRED)
find_package(tl-expected CONFIG REQUIRED)
find_package(certify CONFIG REQUIRED)

include(${CMAKE_CURRENT_LIST_DIR}/launchdarklyTargets.cmake)
