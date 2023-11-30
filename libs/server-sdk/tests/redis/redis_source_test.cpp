#include <gtest/gtest.h>

#include <launchdarkly/server_side/integrations/redis/redis_source.hpp>
#include "data_components/kinds/kinds.hpp"

#include <redis++.h>

#include <boost/json.hpp>
#include <launchdarkly/serialization/json_flag.hpp>

using namespace launchdarkly::server_side::data_systems;
using namespace launchdarkly::server_side::data_components;
using namespace launchdarkly::data_model;

class RedisTests : public ::testing::Test {
   public:
    explicit RedisTests()
        : uri_("tcp://localhost:6379"), prefix_("testprefix"), client_(uri_) {}

    void SetUp() override {
        auto maybe_source = RedisDataSource::Create(uri_, prefix_);
        ASSERT_TRUE(maybe_source);
        source = std::move(*maybe_source);
    }

    testing::AssertionResult PutFlag(Flag const& flag) {
        try {
            client_.hset(prefix_ + ":features", flag.key,
                         serialize(boost::json::value_from(flag)));
            return testing::AssertionSuccess();
        } catch (sw::redis::Error const& e) {
            return testing::AssertionFailure(testing::Message(e.what()));
        }
    }

   protected:
    std::shared_ptr<RedisDataSource> source;

   private:
    std::string const uri_;
    std::string const prefix_;
    sw::redis::Redis client_;
};

TEST_F(RedisTests, Sourc) {
    ASSERT_TRUE(PutFlag(Flag{"foo", 2, true}));

    ASSERT_FALSE(source->Initialized());

    auto all_flags = source->All(FlagKind{});
    ASSERT_TRUE(all_flags.has_value());
    ASSERT_EQ(all_flags->size(), 0);
}
