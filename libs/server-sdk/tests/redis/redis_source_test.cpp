#include <gtest/gtest.h>

#include <launchdarkly/server_side/integrations/redis/redis_source.hpp>
#include "data_components/kinds/kinds.hpp"

using namespace launchdarkly::server_side::data_systems;
using namespace launchdarkly::server_side::data_components;

TEST(RedisTests, ConnectToRedis) {
    auto maybe_source = RedisDataSource::Create("tcp://localhost:6379", "test");
    ASSERT_TRUE(maybe_source);

    auto const source = *maybe_source;

    ASSERT_FALSE(source->Initialized());

    auto all_flags = source->All(FlagKind{});
    ASSERT_TRUE(all_flags.has_value());
    ASSERT_EQ(all_flags->size(), 0);
}
