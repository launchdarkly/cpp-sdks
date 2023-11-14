#include <gtest/gtest.h>

#include "data_systems/lazy_load/sources/redis/redis_source.hpp"

using namespace launchdarkly::server_side::data_systems;
TEST(RedisTests, ConnectToRedis) {
    RedisDataSource source("tcp://localhost:6379", "test");
}
