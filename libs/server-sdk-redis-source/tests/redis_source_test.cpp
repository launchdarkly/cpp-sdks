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
            client_.set(Prefixed("$inited"), "true");
        } catch (sw::redis::Error const& e) {
            FAIL() << e.what();
        }
    }

    void PutFlag(Flag const& flag) const {
        try {
            client_.hset(Prefixed("features"), flag.key,
                         serialize(boost::json::value_from(flag)));
        } catch (sw::redis::Error const& e) {
            FAIL() << e.what();
        }
    }

    void PutDeletedFlag(std::string const& key, std::string const& ts) const {
        try {
            client_.hset(Prefixed("features"), key, ts);
        } catch (sw::redis::Error const& e) {
            FAIL() << e.what();
        }
    }

    void PutDeletedSegment(std::string const& key,
                           std::string const& ts) const {
        try {
            client_.hset(Prefixed("segments"), key, ts);
        } catch (sw::redis::Error const& e) {
            FAIL() << e.what();
        }
    }

    void PutSegment(Segment const& segment) const {
        try {
            client_.hset(Prefixed("segments"), segment.key,
                         serialize(boost::json::value_from(segment)));
        } catch (sw::redis::Error const& e) {
            FAIL() << e.what();
        }
    }

   private:
    std::string Prefixed(std::string const& name) const {
        return prefix_ + ":" + name;
    }

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

    void PutDeletedFlag(std::string const& key, std::string const& ts) {
        auto const client = PrefixedClient(client_, prefix_);
        client.PutDeletedFlag(key, ts);
    }

    void PutDeletedSegment(std::string const& key, std::string const& ts) {
        auto const client = PrefixedClient(client_, prefix_);
        client.PutDeletedSegment(key, ts);
    }

    void PutSegment(Segment const& segment) {
        auto const client = PrefixedClient(client_, prefix_);
        client.PutSegment(segment);
    }

    void WithPrefixedClient(
        std::string const& prefix,
        std::function<void(PrefixedClient const&)> const& f) {
        auto const client = PrefixedClient(client_, prefix);
        f(client);
    }

    void WithPrefixedSource(
        std::string const& prefix,
        std::function<void(RedisDataSource const&)> const& f) const {
        auto maybe_source = RedisDataSource::Create(uri_, prefix);
        ASSERT_TRUE(maybe_source);
        f((*maybe_source->get()));
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
    Flag const flag{"foo", 1, true};
    PutFlag(flag);

    auto const result = source->Get(FlagKind{}, "foo");
    ASSERT_TRUE(result);

    if (auto const f = *result) {
        ASSERT_EQ(f->serializedItem, serialize(boost::json::value_from(flag)));
    } else {
        FAIL() << "expected flag to be found";
    }
}

TEST_F(RedisTests, GetSegment) {
    Segment const segment{"foo", 1};
    PutSegment(segment);

    auto const result = source->Get(SegmentKind{}, "foo");
    ASSERT_TRUE(result);

    if (auto const f = *result) {
        ASSERT_EQ(f->serializedItem,
                  serialize(boost::json::value_from(segment)));
    } else {
        FAIL() << "expected segment to be found";
    }
}

TEST_F(RedisTests, GetMissingFlag) {
    auto const result = source->Get(FlagKind{}, "foo");
    ASSERT_TRUE(result);
    ASSERT_FALSE(*result);
}

TEST_F(RedisTests, GetMissingSegment) {
    auto const result = source->Get(SegmentKind{}, "foo");
    ASSERT_TRUE(result);
    ASSERT_FALSE(*result);
}

TEST_F(RedisTests, GetDeletedFlag) {
    PutDeletedFlag("foo", "foo_tombstone");

    auto const result = source->Get(FlagKind{}, "foo");
    ASSERT_TRUE(result);

    if (auto const f = *result) {
        ASSERT_EQ(f->serializedItem, "foo_tombstone");
    } else {
        FAIL() << "expected tombstone to be present";
    }
}

TEST_F(RedisTests, GetDeletedSegment) {
    PutDeletedSegment("foo", "foo_tombstone");

    auto const result = source->Get(SegmentKind{}, "foo");
    ASSERT_TRUE(result);

    if (auto const f = *result) {
        ASSERT_EQ(f->serializedItem, "foo_tombstone");
    } else {
        FAIL() << "expected tombstone to be present";
    }
}

TEST_F(RedisTests, GetFlagDoesNotFindSegment) {
    PutSegment(Segment{"foo", 1});

    auto const result = source->Get(FlagKind{}, "foo");
    ASSERT_TRUE(result);
    ASSERT_FALSE(*result);
}

TEST_F(RedisTests, GetSegmentDoesNotFindFlag) {
    PutFlag(Flag{"foo", 1, true});

    auto const result = source->Get(SegmentKind{}, "foo");
    ASSERT_TRUE(result);
    ASSERT_FALSE(*result);
}

TEST_F(RedisTests, GetAllSegmentsWhenEmpty) {
    auto const result = source->All(SegmentKind{});
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->empty());
}

TEST_F(RedisTests, GetAllFlagsWhenEmpty) {
    auto const result = source->All(FlagKind{});
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->empty());
}

TEST_F(RedisTests, GetAllFlags) {
    Flag const flag1{"foo", 1, true};
    Flag const flag2{"bar", 2, false};

    PutFlag(flag1);
    PutFlag(flag2);
    PutDeletedFlag("baz", "baz_tombstone");

    auto const result = source->All(FlagKind{});
    ASSERT_TRUE(result);
    ASSERT_EQ(result->size(), 3);

    auto const& flags = *result;
    auto const flag1_it = flags.find("foo");
    ASSERT_NE(flag1_it, flags.end());
    ASSERT_EQ(flag1_it->second.serializedItem,
              serialize(boost::json::value_from(flag1)));

    auto const flag2_it = flags.find("bar");
    ASSERT_NE(flag2_it, flags.end());
    ASSERT_EQ(flag2_it->second.serializedItem,
              serialize(boost::json::value_from(flag2)));

    auto const flag3_it = flags.find("baz");
    ASSERT_NE(flag3_it, flags.end());
    ASSERT_EQ(flag3_it->second.serializedItem, "baz_tombstone");
}

TEST_F(RedisTests, InitializedPrefixIndependence) {
    WithPrefixedClient("not_our_prefix", [&](auto const& client) {
        client.Init();
        ASSERT_FALSE(source->Initialized());
    });

    WithPrefixedClient("TestPrefix", [&](auto const& client) {
        client.Init();
        ASSERT_FALSE(source->Initialized());
    });

    WithPrefixedClient("stillnotprefix", [&](auto const& client) {
        client.Init();
        ASSERT_FALSE(source->Initialized());
    });
}

TEST_F(RedisTests, SegmentPrefixIndependence) {
    auto MakeSegment = [](std::uint64_t const version) {
        return Segment{"foo", version};
    };

    auto PrefixName = [](std::uint64_t const version) {
        return "prefix" + std::to_string(version);
    };

    auto ValidateSegment = [&](ISerializedDataReader::GetResult const& result,
                               std::size_t i) {
        ASSERT_TRUE(result);
        if (auto const f = *result) {
            ASSERT_EQ(f->serializedItem,
                      serialize(boost::json::value_from(MakeSegment(i))));
        } else {
            FAIL() << "expected segment to be found under " << PrefixName(i);
        }
    };

    constexpr std::size_t kPrefixCount = 10;

    // Setup the same segment key (with different versions) under kPrefixCount
    // prefixes. This will allow us to verify that the prefixed clients only
    // "see" the single segment and not the ones living under different
    // prefixes.

    for (std::size_t i = 0; i < kPrefixCount; i++) {
        WithPrefixedClient(PrefixName(i), [&](auto const& client) {
            client.PutSegment(MakeSegment(i));
        });
    }

    for (std::size_t i = 0; i < kPrefixCount; i++) {
        WithPrefixedSource(PrefixName(i), [&](auto const& source) {
            // Checks that the string that was stored for segment #i is the
            // same one that was retrieved.
            ValidateSegment(source.Get(SegmentKind{}, "foo"), i);

            // Sanity check that the other segments are not visible.
            auto all = source.All(SegmentKind{});
            ASSERT_TRUE(all);
            ASSERT_EQ(all->size(), 1);
        });
    }
}

TEST_F(RedisTests, FlagPrefixIndependence) {
    auto MakeFlag = [](std::uint64_t const version) {
        return Flag{"foo", version, true};
    };

    auto PrefixName = [](std::uint64_t const version) {
        return "prefix" + std::to_string(version);
    };

    auto ValidateFlag = [&](ISerializedDataReader::GetResult const& result,
                            std::size_t i) {
        ASSERT_TRUE(result);
        if (auto const f = *result) {
            ASSERT_EQ(f->serializedItem,
                      serialize(boost::json::value_from(MakeFlag(i))));
        } else {
            FAIL() << "expected flag to be found under " << PrefixName(i);
        }
    };

    constexpr std::size_t kPrefixCount = 10;

    // Setup the same flag key (with different versions) under kPrefixCount
    // prefixes. This will allow us to verify that the prefixed clients only
    // "see" the single flag and not the ones living under different prefixes.

    for (std::size_t i = 0; i < kPrefixCount; i++) {
        WithPrefixedClient(PrefixName(i), [&](auto const& client) {
            client.PutFlag(MakeFlag(i));
        });
    }

    for (std::size_t i = 0; i < kPrefixCount; i++) {
        WithPrefixedSource(PrefixName(i), [&](auto const& source) {
            // Checks that the string that was stored for flag #i is the
            // same one that was retrieved.
            ValidateFlag(source.Get(FlagKind{}, "foo"), i);

            // Sanity check that the other flags are not visible.
            auto all = source.All(FlagKind{});
            ASSERT_TRUE(all);
            ASSERT_EQ(all->size(), 1);
        });
    }
}

TEST_F(RedisTests, FlagAndSegmentCanCoexistWithSameKey) {
    Flag const flag_in{"foo", 1, true};
    Segment const segment_in{"foo", 1};

    PutFlag(flag_in);
    PutSegment(segment_in);

    auto flag = source->Get(FlagKind{}, "foo");
    ASSERT_TRUE(flag);
    ASSERT_EQ((*flag)->serializedItem,
              serialize(boost::json::value_from(flag_in)));

    auto segment = source->Get(SegmentKind{}, "foo");
    ASSERT_TRUE(segment);
    ASSERT_EQ((*segment)->serializedItem,
              serialize(boost::json::value_from(segment_in)));
}

TEST(RedisErrorTests, InvalidURIs) {
    std::vector<std::string> const uris = {"nope, not a redis URI",
                                           "http://foo",
                                           "foo.com"
                                           ""};

    for (auto const& uri : uris) {
        auto const source = RedisDataSource::Create(uri, "prefix");
        ASSERT_FALSE(source);
    }
}

TEST(RedisErrorTests, ValidURIs) {
    std::vector<std::string> const uris = {
        "tcp://127.0.0.1:6666",
        "tcp://127.0.0.1",
        "tcp://pass@127.0.0.1",
        "tcp://127.0.0.1:6379/2",
        "tcp://127.0.0.1:6379/2?keep_alive=true",
        "tcp://127.0.0.1?socket_timeout=50ms&connect_timeout=1s",
        "unix://path/to/socket"};
    for (auto const& uri : uris) {
        auto const source = RedisDataSource::Create(uri, "prefix");
        ASSERT_TRUE(source);
    }
}

TEST(RedisErrorTests, GetReturnsErrorAndNoExceptionThrown) {
    auto const maybe_source = RedisDataSource::Create(
        "tcp://foobar:1000" /* no redis service here */, "prefix");
    ASSERT_TRUE(maybe_source);

    auto const source = *maybe_source;

    auto const get_initialized = source->Initialized();
    ASSERT_FALSE(get_initialized);

    auto const get_flag = source->Get(FlagKind{}, "foo");
    ASSERT_FALSE(get_flag);

    auto const get_segment = source->Get(SegmentKind{}, "foo");
    ASSERT_FALSE(get_segment);

    auto const get_all_flag = source->All(FlagKind{});
    ASSERT_FALSE(get_all_flag);

    auto const get_all_segment = source->All(SegmentKind{});
    ASSERT_FALSE(get_all_segment);
}
