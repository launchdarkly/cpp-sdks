#include <gtest/gtest.h>

#include <launchdarkly/server_side/integrations/data_reader/kinds.hpp>
#include <launchdarkly/server_side/integrations/redis/redis_source.hpp>

#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

#include <boost/json.hpp>

#include <redis++.h>

using namespace launchdarkly::server_side::integrations;
using namespace launchdarkly::data_model;

class PrefixedClient {
   public:
    PrefixedClient(sw::redis::Redis& client, std::string const& prefix)
        : client_(client), prefix_(prefix) {}

    void Init() const {
        try {
            client_.set(prefix_ + ":$inited", "true");
        } catch (sw::redis::Error const& e) {
            FAIL() << e.what();
        }
    }

    void PutFlag(Flag const& flag) const {
        try {
            client_.hset(prefix_ + ":features", flag.key,
                         serialize(boost::json::value_from(flag)));
        } catch (sw::redis::Error const& e) {
            FAIL() << e.what();
        }
    }

    void PutSegment(Segment const& segment) const {
        try {
            client_.hset(prefix_ + ":segments", segment.key,
                         serialize(boost::json::value_from(segment)));
        } catch (sw::redis::Error const& e) {
            FAIL() << e.what();
        }
    }

    void Clear() const {
        try {
            std::vector<std::pair<std::string, std::string>> output;
            client_.keys(prefix_ + ":*", std::back_inserter(output));
            for (auto const& [key, _] : output) {
                client_.del(key);
            }
        } catch (sw::redis::Error const& e) {
            FAIL() << e.what();
        }
    }

   private:
    sw::redis::Redis& client_;
    std::string const& prefix_;
};

class RedisTests : public ::testing::Test {
   public:
    explicit RedisTests()
        : uri_("tcp://localhost:6379"), prefix_("testprefix"), client_(uri_) {}

    void SetUp() override {
        try {
            client_.flushdb();
        } catch (sw::redis::Error const& e) {
            FAIL() << "couldn't clear Redis: " << e.what();
        }

        auto maybe_source = RedisDataSource::Create(uri_, prefix_);
        ASSERT_TRUE(maybe_source);
        source = std::move(*maybe_source);
    }

    void Init() {
        auto const client = PrefixedClient(client_, prefix_);
        client.Init();
    }

    void PutFlag(Flag const& flag) {
        auto const client = PrefixedClient(client_, prefix_);
        client.PutFlag(flag);
    }

    void PutSegment(Segment const& segment) {
        auto const client = PrefixedClient(client_, prefix_);
        client.PutSegment(segment);
    }

    void Clear() {
        auto const client = PrefixedClient(client_, prefix_);
        client.Clear();
    }

    void WithPrefix(std::string const& prefix,
                    std::function<void(PrefixedClient const&)> const& f) {
        auto const client = PrefixedClient(client_, prefix);
        f(client);
    }

   protected:
    std::shared_ptr<RedisDataSource> source;

   private:
    std::string const uri_;
    std::string const prefix_;
    sw::redis::Redis client_;
};

TEST_F(RedisTests, RedisEmptyIsNotInitialized) {
    ASSERT_FALSE(source->Initialized());

    auto all_flags = source->All(FlagKind{});
    ASSERT_TRUE(all_flags.has_value());
    ASSERT_TRUE(all_flags->empty());

    auto all_segments = source->All(SegmentKind{});
    ASSERT_TRUE(all_segments.has_value());
    ASSERT_TRUE(all_flags->empty());
}

TEST_F(RedisTests, ChecksInitialized) {
    ASSERT_FALSE(source->Initialized());
    Init();
    ASSERT_TRUE(source->Initialized());
}

TEST_F(RedisTests, GetFlag) {
    PutFlag(Flag{"foo", 1, true});

    auto const result = source->Get(FlagKind{}, "foo");
    ASSERT_TRUE(result);

    ASSERT_FALSE(result->deleted);
    ASSERT_TRUE(result->serializedItem);
}

TEST_F(RedisTests, GetFlagDoesNotFindSegment) {
    PutSegment(Segment{"foo", 1});

    auto const result = source->Get(FlagKind{}, "foo");
    ASSERT_FALSE(result);
}

TEST_F(RedisTests, GetSegment) {
    PutSegment(Segment{"foo", 1});

    auto const result = source->Get(SegmentKind{}, "foo");
    ASSERT_TRUE(result);

    ASSERT_FALSE(result->deleted);
    ASSERT_TRUE(result->serializedItem);
}

TEST_F(RedisTests, GetSegmentDoesNotFindFlag) {
    PutFlag(Flag{"foo", 1, true});

    auto const result = source->Get(SegmentKind{}, "foo");
    ASSERT_FALSE(result);
}

TEST_F(RedisTests, ChecksInitializedPrefixIndependence) {
    WithPrefix("not_our_prefix", [&](auto const& client) {
        client.Init();
        ASSERT_FALSE(source->Initialized());
    });

    WithPrefix("TestPrefix", [&](auto const& client) {
        client.Init();
        ASSERT_FALSE(source->Initialized());
    });

    WithPrefix("stillnotprefix", [&](auto const& client) {
        client.Init();
        ASSERT_FALSE(source->Initialized());
    });
}