#include <gtest/gtest.h>

#include <launchdarkly/server_side/integrations/redis/redis_big_segment_store.hpp>

#include <sw/redis++/redis++.h>

#include <chrono>
#include <string>
#include <vector>

using namespace launchdarkly::server_side::integrations;

namespace {

class RedisBigSegmentTests : public ::testing::Test {
   public:
    RedisBigSegmentTests()
        : uri_("redis://localhost:6379"),
          prefix_("testprefix"),
          client_(uri_) {}

    void SetUp() override {
        try {
            client_.flushdb();
        } catch (sw::redis::Error const& e) {
            FAIL() << "couldn't clear Redis: " << e.what();
        }

        auto maybe_store = RedisBigSegmentStore::Create(uri_, prefix_);
        ASSERT_TRUE(maybe_store);
        store_ = std::move(*maybe_store);
    }

    void AddIncludes(std::string const& context_hash,
                     std::vector<std::string> const& refs) {
        AddIncludesUnderPrefix(prefix_, context_hash, refs);
    }

    void AddIncludesUnderPrefix(std::string const& prefix,
                                std::string const& context_hash,
                                std::vector<std::string> const& refs) {
        auto const key = prefix + ":big_segment_include:" + context_hash;
        for (auto const& ref : refs) {
            client_.sadd(key, ref);
        }
    }

    void AddExcludes(std::string const& context_hash,
                     std::vector<std::string> const& refs) {
        auto const key = prefix_ + ":big_segment_exclude:" + context_hash;
        for (auto const& ref : refs) {
            client_.sadd(key, ref);
        }
    }

    void SetSyncTime(std::int64_t millis) {
        SetSyncTimeRaw(std::to_string(millis));
    }

    void SetSyncTimeRaw(std::string const& value) {
        client_.set(prefix_ + ":big_segments_synchronized_on", value);
    }

   protected:
    std::unique_ptr<RedisBigSegmentStore> store_;
    std::string const uri_;
    std::string const prefix_;
    sw::redis::Redis client_;
};

}  // namespace

TEST_F(RedisBigSegmentTests, EmptyStoreReturnsEmptyMembership) {
    auto const result = store_->GetMembership("nobody");
    ASSERT_TRUE(result);
    ASSERT_FALSE(result->CheckMembership("seg1.g1").has_value());
}

TEST_F(RedisBigSegmentTests, EmptyStoreReturnsNoMetadata) {
    auto const result = store_->GetMetadata();
    ASSERT_TRUE(result);
    ASSERT_FALSE(result->has_value());
}

TEST_F(RedisBigSegmentTests, GetMembershipWithIncludesOnly) {
    AddIncludes("alice", {"seg1.g1", "seg2.g3"});

    auto const result = store_->GetMembership("alice");
    ASSERT_TRUE(result);

    ASSERT_EQ(result->CheckMembership("seg1.g1"), true);
    ASSERT_EQ(result->CheckMembership("seg2.g3"), true);
    ASSERT_FALSE(result->CheckMembership("seg3.g1").has_value());
}

TEST_F(RedisBigSegmentTests, GetMembershipWithExcludesOnly) {
    AddExcludes("bob", {"seg1.g1"});

    auto const result = store_->GetMembership("bob");
    ASSERT_TRUE(result);
    ASSERT_EQ(result->CheckMembership("seg1.g1"), false);
}

TEST_F(RedisBigSegmentTests, GetMembershipInclusionWinsOverExclusion) {
    AddIncludes("carol", {"seg.g1"});
    AddExcludes("carol", {"seg.g1"});

    auto const result = store_->GetMembership("carol");
    ASSERT_TRUE(result);
    ASSERT_EQ(result->CheckMembership("seg.g1"), true);
}

TEST_F(RedisBigSegmentTests, GetMembershipIsPrefixScoped) {
    // Write under a different prefix; same-named test store should not see it.
    AddIncludesUnderPrefix("otherprefix", "alice", {"seg1.g1"});

    auto const result = store_->GetMembership("alice");
    ASSERT_TRUE(result);
    ASSERT_FALSE(result->CheckMembership("seg1.g1").has_value());
}

TEST_F(RedisBigSegmentTests, GetMembershipWithEmptyPrefix) {
    auto maybe_store = RedisBigSegmentStore::Create(uri_, "");
    ASSERT_TRUE(maybe_store) << maybe_store.error();
    auto const store = std::move(*maybe_store);

    // Relay writes keys with no leading colon when no prefix is configured.
    client_.sadd("big_segment_include:alice", "seg1.g1");

    auto const result = store->GetMembership("alice");
    ASSERT_TRUE(result);
    ASSERT_EQ(result->CheckMembership("seg1.g1"), true);
}

TEST_F(RedisBigSegmentTests, GetMetadataWithEmptyPrefix) {
    auto maybe_store = RedisBigSegmentStore::Create(uri_, "");
    ASSERT_TRUE(maybe_store) << maybe_store.error();
    auto const store = std::move(*maybe_store);

    client_.set("big_segments_synchronized_on", "1700000000000");

    auto const result = store->GetMetadata();
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->has_value());
    ASSERT_EQ(result->value().last_up_to_date,
              std::chrono::system_clock::time_point{
                  std::chrono::milliseconds{1700000000000LL}});
}

TEST_F(RedisBigSegmentTests, GetMetadataReturnsSyncTime) {
    SetSyncTime(1700000000000LL);

    auto const result = store_->GetMetadata();
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->has_value());
    ASSERT_EQ(result->value().last_up_to_date,
              std::chrono::system_clock::time_point{
                  std::chrono::milliseconds{1700000000000LL}});
}

TEST_F(RedisBigSegmentTests, GetMetadataRejectsMalformedSyncTime) {
    SetSyncTimeRaw("not-a-number");

    auto const result = store_->GetMetadata();
    ASSERT_FALSE(result);
}

TEST(RedisBigSegmentStoreErrors, GetReturnsErrorOnUnreachableServer) {
    auto maybe_store =
        RedisBigSegmentStore::Create("tcp://foobar:1000", "prefix");
    ASSERT_TRUE(maybe_store);

    auto const store = std::move(*maybe_store);
    ASSERT_FALSE(store->GetMembership("anyone"));
    ASSERT_FALSE(store->GetMetadata());
}
