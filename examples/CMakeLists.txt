add_subdirectory(hello-c-client)
add_subdirectory(hello-cpp-client)
add_subdirectory(hello-cpp-server)
add_subdirectory(hello-c-server)
add_subdirectory(client-and-server-coexistence)

if (LD_BUILD_REDIS_SUPPORT)
    add_subdirectory(hello-cpp-server-redis)
endif ()
