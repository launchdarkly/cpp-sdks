#include <gtest/gtest.h>

#include <data_components/big_segments/membership_cache.hpp>

#include <chrono>
#include <functional>

using launchdarkly::server_side::data_components::MembershipCache;
using launchdarkly::server_side::integrations::Membership;
using namespace std::chrono_literals;

namespace {

// A clock whose value the test advances by hand, so TTL behavior is
// deterministic rather than dependent on wall-clock timing.
class FakeClock {
   public:
    void Advance(std::chrono::milliseconds by) { now_ += by; }

    std::function<std::chrono::steady_clock::time_point()> AsFn() {
        return [this] { return now_; };
    }

   private:
    std::chrono::steady_clock::time_point now_{};
};

Membership MemberOf(std::string const& segment_ref) {
    return Membership::FromSegmentRefs({segment_ref}, {});
}

}  // namespace

TEST(MembershipCacheTest, MissOnAbsentKey) {
    MembershipCache cache(10, 1000ms);
    EXPECT_FALSE(cache.Get("nobody").has_value());
}

TEST(MembershipCacheTest, ReturnsStoredValue) {
    MembershipCache cache(10, 1000ms);
    cache.Set("a", MemberOf("seg.g1"));

    auto result = cache.Get("a");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(true, result->CheckMembership("seg.g1"));
}

TEST(MembershipCacheTest, SetReplacesExistingValue) {
    MembershipCache cache(10, 1000ms);
    cache.Set("a", MemberOf("first.g1"));
    cache.Set("a", MemberOf("second.g1"));

    auto result = cache.Get("a");
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->CheckMembership("first.g1").has_value());
    EXPECT_EQ(true, result->CheckMembership("second.g1"));
    EXPECT_EQ(1u, cache.Size());
}

TEST(MembershipCacheTest, EvictsLeastRecentlyUsedAtCapacity) {
    MembershipCache cache(2, 1000ms);
    cache.Set("a", MemberOf("a.g1"));
    cache.Set("b", MemberOf("b.g1"));
    cache.Set("c", MemberOf("c.g1"));  // evicts "a", the LRU

    EXPECT_EQ(2u, cache.Size());
    EXPECT_FALSE(cache.Get("a").has_value());
    EXPECT_TRUE(cache.Get("b").has_value());
    EXPECT_TRUE(cache.Get("c").has_value());
}

TEST(MembershipCacheTest, GetRefreshesRecency) {
    MembershipCache cache(2, 1000ms);
    cache.Set("a", MemberOf("a.g1"));
    cache.Set("b", MemberOf("b.g1"));

    // Touch "a" so "b" becomes the least-recently-used.
    EXPECT_TRUE(cache.Get("a").has_value());

    cache.Set("c", MemberOf("c.g1"));  // evicts "b" now, not "a"

    EXPECT_TRUE(cache.Get("a").has_value());
    EXPECT_FALSE(cache.Get("b").has_value());
    EXPECT_TRUE(cache.Get("c").has_value());
}

TEST(MembershipCacheTest, ExpiresAfterTtlAndIsRemoved) {
    FakeClock clock;
    MembershipCache cache(10, 1000ms, clock.AsFn());
    cache.Set("a", MemberOf("a.g1"));

    clock.Advance(999ms);
    EXPECT_TRUE(cache.Get("a").has_value());

    clock.Advance(1ms);  // now exactly at the TTL boundary
    EXPECT_FALSE(cache.Get("a").has_value());
    EXPECT_EQ(0u, cache.Size());  // the expired entry is dropped, not retained
}

TEST(MembershipCacheTest, SetResetsExpiration) {
    FakeClock clock;
    MembershipCache cache(10, 1000ms, clock.AsFn());
    cache.Set("a", MemberOf("a.g1"));

    clock.Advance(600ms);
    cache.Set("a", MemberOf("a.g1"));  // re-insert resets the TTL

    clock.Advance(600ms);  // 1200ms since first set, 600ms since the second
    EXPECT_TRUE(cache.Get("a").has_value());
}

TEST(MembershipCacheTest, ClearRemovesEverything) {
    MembershipCache cache(10, 1000ms);
    cache.Set("a", MemberOf("a.g1"));
    cache.Set("b", MemberOf("b.g1"));

    cache.Clear();

    EXPECT_EQ(0u, cache.Size());
    EXPECT_FALSE(cache.Get("a").has_value());
    EXPECT_FALSE(cache.Get("b").has_value());
}
