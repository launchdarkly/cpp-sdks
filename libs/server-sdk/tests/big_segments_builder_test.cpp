#include <gtest/gtest.h>

#include <launchdarkly/server_side/integrations/big_segments/big_segment_store_types.hpp>
#include <launchdarkly/server_side/integrations/big_segments/ibig_segment_store.hpp>

#include "config/builders/big_segments_builder.hpp"

#include <chrono>
#include <memory>

using launchdarkly::server_side::config::builders::BigSegmentsBuilder;
using launchdarkly::server_side::integrations::IBigSegmentStore;
using launchdarkly::server_side::integrations::Membership;
using launchdarkly::server_side::integrations::StoreMetadata;

namespace {

using namespace std::chrono_literals;

// Minimal stub used only to obtain a shared_ptr<IBigSegmentStore>. The builder
// never invokes the store; it only stores the pointer for later use by the
// wrapper, so the methods here are unreachable in these tests.
class StubStore final : public IBigSegmentStore {
   public:
    GetMembershipResult GetMembership(
        std::string const& /*context_hash*/) const override {
        return Membership::FromSegmentRefs({}, {});
    }
    GetMetadataResult GetMetadata() const override {
        return std::optional<StoreMetadata>{};
    }
};

std::shared_ptr<IBigSegmentStore> MakeStubStore() {
    return std::make_shared<StubStore>();
}

}  // namespace

TEST(BigSegmentsBuilderTest, DefaultsMatchSpec) {
    auto store = MakeStubStore();
    auto const cfg = BigSegmentsBuilder(store).Build();

    EXPECT_EQ(cfg.context_cache_size, 1000u);
    EXPECT_EQ(cfg.context_cache_time, 5s);
    EXPECT_EQ(cfg.status_poll_interval, 5s);
    EXPECT_EQ(cfg.stale_after, 2min);
}

TEST(BigSegmentsBuilderTest, BuildPreservesStoreIdentity) {
    auto store = MakeStubStore();
    auto const cfg = BigSegmentsBuilder(store).Build();
    EXPECT_EQ(cfg.store.get(), store.get());
}

TEST(BigSegmentsBuilderTest, AcceptsNullStore) {
    // The builder doesn't validate the store; downstream components treat a
    // null store as "Big Segments not configured".
    auto const cfg = BigSegmentsBuilder(nullptr).Build();
    EXPECT_EQ(cfg.store, nullptr);
}

TEST(BigSegmentsBuilderTest, SettersOverrideEachField) {
    auto store = MakeStubStore();
    auto const cfg = BigSegmentsBuilder(store)
                         .ContextCacheSize(7)
                         .ContextCacheTime(11s)
                         .StatusPollInterval(13s)
                         .StaleAfter(60s)
                         .Build();

    EXPECT_EQ(cfg.context_cache_size, 7u);
    EXPECT_EQ(cfg.context_cache_time, 11s);
    EXPECT_EQ(cfg.status_poll_interval, 13s);
    EXPECT_EQ(cfg.stale_after, 60s);
}

TEST(BigSegmentsBuilderTest, ZeroDurationsAreCoercedToDefaults) {
    auto store = MakeStubStore();
    auto const cfg = BigSegmentsBuilder(store)
                         .ContextCacheTime(0ms)
                         .StatusPollInterval(0ms)
                         .StaleAfter(0ms)
                         .Build();

    EXPECT_EQ(cfg.context_cache_time, 5s);
    EXPECT_EQ(cfg.status_poll_interval, 5s);
    EXPECT_EQ(cfg.stale_after, 2min);
}

TEST(BigSegmentsBuilderTest, NegativeDurationsAreCoercedToDefaults) {
    auto store = MakeStubStore();
    auto const cfg = BigSegmentsBuilder(store)
                         .ContextCacheTime(-1ms)
                         .StatusPollInterval(-1ms)
                         .StaleAfter(-1ms)
                         .Build();

    EXPECT_EQ(cfg.context_cache_time, 5s);
    EXPECT_EQ(cfg.status_poll_interval, 5s);
    EXPECT_EQ(cfg.stale_after, 2min);
}

TEST(BigSegmentsBuilderTest, BuildClampsPollIntervalToStaleAfter) {
    // When poll interval > stale-after, clamp poll to stale-after so the
    // SDK detects staleness within one poll cycle.
    auto store = MakeStubStore();
    auto const cfg = BigSegmentsBuilder(store)
                         .StatusPollInterval(10s)
                         .StaleAfter(3s)
                         .Build();

    EXPECT_EQ(cfg.status_poll_interval, 3s);
    EXPECT_EQ(cfg.stale_after, 3s);
}

TEST(BigSegmentsBuilderTest, BuildPreservesPollIntervalWhenWithinStaleAfter) {
    auto store = MakeStubStore();
    auto const cfg = BigSegmentsBuilder(store)
                         .StatusPollInterval(3s)
                         .StaleAfter(10s)
                         .Build();

    EXPECT_EQ(cfg.status_poll_interval, 3s);
    EXPECT_EQ(cfg.stale_after, 10s);
}

TEST(BigSegmentsBuilderTest, BuildIsRepeatable) {
    auto store = MakeStubStore();
    BigSegmentsBuilder builder(store);
    builder.ContextCacheSize(42).ContextCacheTime(2s);

    auto const cfg1 = builder.Build();
    auto const cfg2 = builder.Build();

    EXPECT_EQ(cfg1.context_cache_size, cfg2.context_cache_size);
    EXPECT_EQ(cfg1.context_cache_time, cfg2.context_cache_time);
    EXPECT_EQ(cfg1.store.get(), cfg2.store.get());
}
