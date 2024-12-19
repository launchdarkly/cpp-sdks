# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyHelloCServerRedis
        VERSION 0.1
        DESCRIPTION "LaunchDarkly Hello C Server-side SDK with Redis source"
        LANGUAGES C
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(hello-c-server-redis-source main.c)
target_link_libraries(hello-c-server-redis-source PRIVATE launchdarkly::server_redis_source Threads::Threads)
