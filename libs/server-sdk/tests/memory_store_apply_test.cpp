#include <gtest/gtest.h>

#include <data_components/memory_store/memory_store.hpp>
#include <launchdarkly/data_model/fdv2_change.hpp>

using namespace launchdarkly::data_model;
using namespace launchdarkly::server_side::data_components;

// ---------------------------------------------------------------------------
// kNone tests
// ---------------------------------------------------------------------------

TEST(MemoryStoreApplyTest, ApplyNone_IsNoOp) {
    MemoryStore store;
    Flag flag_a;
    flag_a.version = 1;
    flag_a.key = "flagA";

    Segment seg_a;
    seg_a.version = 1;
    seg_a.key = "segA";

    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a)}},
        std::unordered_map<std::string, SegmentDescriptor>{
            {"segA", SegmentDescriptor(seg_a)}},
    });

    auto result =
        store.Apply(FDv2ChangeSet{FDv2ChangeSet::Type::kNone, {}, Selector{}});

    auto fetched_flag = store.GetFlag("flagA");
    ASSERT_TRUE(fetched_flag);
    EXPECT_EQ(1u, fetched_flag->version);
    auto fetched_seg = store.GetSegment("segA");
    ASSERT_TRUE(fetched_seg);
    EXPECT_EQ(1u, fetched_seg->version);

    EXPECT_TRUE(result.flags.empty());
    EXPECT_TRUE(result.segments.empty());
}

TEST(MemoryStoreApplyTest, ApplyNone_DoesNotInitialize) {
    MemoryStore store;
    std::ignore = store.Apply(FDv2ChangeSet{FDv2ChangeSet::Type::kNone, {}, Selector{}});
    EXPECT_FALSE(store.Initialized());
}

// ---------------------------------------------------------------------------
// kFull tests
// ---------------------------------------------------------------------------

TEST(MemoryStoreApplyTest, ApplyFull_SetsInitialized) {
    MemoryStore store;
    ASSERT_FALSE(store.Initialized());
    std::ignore = store.Apply(FDv2ChangeSet{FDv2ChangeSet::Type::kFull, {}, Selector{}});
    EXPECT_TRUE(store.Initialized());
}

TEST(MemoryStoreApplyTest, ApplyFull_WithFlag) {
    MemoryStore store;
    Flag flag_a;
    flag_a.version = 1;
    flag_a.key = "flagA";

    auto result = store.Apply(FDv2ChangeSet{
        FDv2ChangeSet::Type::kFull,
        std::vector<FDv2Change>{{"flagA", FlagDescriptor(flag_a)}},
        Selector{},
    });

    auto fetched = store.GetFlag("flagA");
    ASSERT_TRUE(fetched);
    EXPECT_TRUE(fetched->item.has_value());
    EXPECT_EQ("flagA", fetched->item->key);
    EXPECT_EQ(1u, fetched->version);

    ASSERT_EQ(1u, result.flags.size());
    EXPECT_EQ(1u, result.flags.count("flagA"));
    EXPECT_TRUE(result.segments.empty());
}

TEST(MemoryStoreApplyTest, ApplyFull_WithSegment) {
    MemoryStore store;
    Segment seg_a;
    seg_a.version = 1;
    seg_a.key = "segA";

    auto result = store.Apply(FDv2ChangeSet{
        FDv2ChangeSet::Type::kFull,
        std::vector<FDv2Change>{{"segA", SegmentDescriptor(seg_a)}},
        Selector{},
    });

    auto fetched = store.GetSegment("segA");
    ASSERT_TRUE(fetched);
    EXPECT_TRUE(fetched->item.has_value());
    EXPECT_EQ("segA", fetched->item->key);
    EXPECT_EQ(1u, fetched->version);

    EXPECT_TRUE(result.flags.empty());
    ASSERT_EQ(1u, result.segments.size());
    EXPECT_EQ(1u, result.segments.count("segA"));
}

TEST(MemoryStoreApplyTest, ApplyFull_ClearsExistingFlags) {
    MemoryStore store;
    Flag flag_a;
    flag_a.version = 1;
    flag_a.key = "flagA";

    Flag flag_b;
    flag_b.version = 1;
    flag_b.key = "flagB";

    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a)},
            {"flagB", FlagDescriptor(flag_b)}},
        std::unordered_map<std::string, SegmentDescriptor>(),
    });

    Flag flag_c;
    flag_c.version = 1;
    flag_c.key = "flagC";

    auto result = store.Apply(FDv2ChangeSet{
        FDv2ChangeSet::Type::kFull,
        std::vector<FDv2Change>{{"flagC", FlagDescriptor(flag_c)}},
        Selector{},
    });

    EXPECT_FALSE(store.GetFlag("flagA"));
    EXPECT_FALSE(store.GetFlag("flagB"));
    ASSERT_TRUE(store.GetFlag("flagC"));
    EXPECT_EQ("flagC", store.GetFlag("flagC")->item->key);

    // Cleared keys (flagA, flagB) and new key (flagC) all reported as changed.
    ASSERT_EQ(3u, result.flags.size());
    EXPECT_EQ(1u, result.flags.count("flagA"));
    EXPECT_EQ(1u, result.flags.count("flagB"));
    EXPECT_EQ(1u, result.flags.count("flagC"));
    EXPECT_TRUE(result.segments.empty());
}

TEST(MemoryStoreApplyTest, ApplyFull_ClearsExistingSegments) {
    MemoryStore store;
    Segment seg_a;
    seg_a.version = 1;
    seg_a.key = "segA";

    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>(),
        std::unordered_map<std::string, SegmentDescriptor>{
            {"segA", SegmentDescriptor(seg_a)}},
    });

    Segment seg_b;
    seg_b.version = 1;
    seg_b.key = "segB";

    auto result = store.Apply(FDv2ChangeSet{
        FDv2ChangeSet::Type::kFull,
        std::vector<FDv2Change>{{"segB", SegmentDescriptor(seg_b)}},
        Selector{},
    });

    EXPECT_FALSE(store.GetSegment("segA"));
    ASSERT_TRUE(store.GetSegment("segB"));

    // Cleared key (segA) and new key (segB) both reported as changed.
    EXPECT_TRUE(result.flags.empty());
    ASSERT_EQ(2u, result.segments.size());
    EXPECT_EQ(1u, result.segments.count("segA"));
    EXPECT_EQ(1u, result.segments.count("segB"));
}

TEST(MemoryStoreApplyTest, ApplyFull_EmptyChangeSetClearsStore) {
    MemoryStore store;
    Flag flag_a;
    flag_a.version = 1;
    flag_a.key = "flagA";

    Segment seg_a;
    seg_a.version = 1;
    seg_a.key = "segA";

    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a)}},
        std::unordered_map<std::string, SegmentDescriptor>{
            {"segA", SegmentDescriptor(seg_a)}},
    });

    auto result =
        store.Apply(FDv2ChangeSet{FDv2ChangeSet::Type::kFull, {}, Selector{}});

    EXPECT_EQ(0u, store.AllFlags().size());
    EXPECT_EQ(0u, store.AllSegments().size());

    ASSERT_EQ(1u, result.flags.size());
    EXPECT_EQ(1u, result.flags.count("flagA"));
    ASSERT_EQ(1u, result.segments.size());
    EXPECT_EQ(1u, result.segments.count("segA"));
}

TEST(MemoryStoreApplyTest, ApplyFull_WithFlagTombstone) {
    MemoryStore store;

    auto result = store.Apply(FDv2ChangeSet{
        FDv2ChangeSet::Type::kFull,
        std::vector<FDv2Change>{{"flagA", FlagDescriptor(Tombstone(5))}},
        Selector{},
    });

    auto fetched = store.GetFlag("flagA");
    ASSERT_TRUE(fetched);
    EXPECT_EQ(5u, fetched->version);
    EXPECT_FALSE(fetched->item.has_value());

    ASSERT_EQ(1u, result.flags.size());
    EXPECT_EQ(1u, result.flags.count("flagA"));
    EXPECT_TRUE(result.segments.empty());
}

TEST(MemoryStoreApplyTest, ApplyFull_WithSegmentTombstone) {
    MemoryStore store;

    auto result = store.Apply(FDv2ChangeSet{
        FDv2ChangeSet::Type::kFull,
        std::vector<FDv2Change>{{"segA", SegmentDescriptor(Tombstone(3))}},
        Selector{},
    });

    auto fetched = store.GetSegment("segA");
    ASSERT_TRUE(fetched);
    EXPECT_EQ(3u, fetched->version);
    EXPECT_FALSE(fetched->item.has_value());

    EXPECT_TRUE(result.flags.empty());
    ASSERT_EQ(1u, result.segments.size());
    EXPECT_EQ(1u, result.segments.count("segA"));
}

// ---------------------------------------------------------------------------
// kPartial tests
// ---------------------------------------------------------------------------

TEST(MemoryStoreApplyTest, ApplyPartial_UpsertsNewFlag) {
    MemoryStore store;
    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>(),
        std::unordered_map<std::string, SegmentDescriptor>(),
    });

    Flag flag_a;
    flag_a.version = 1;
    flag_a.key = "flagA";

    auto result = store.Apply(FDv2ChangeSet{
        FDv2ChangeSet::Type::kPartial,
        std::vector<FDv2Change>{{"flagA", FlagDescriptor(flag_a)}},
        Selector{},
    });

    auto fetched = store.GetFlag("flagA");
    ASSERT_TRUE(fetched);
    EXPECT_TRUE(fetched->item.has_value());
    EXPECT_EQ("flagA", fetched->item->key);
    EXPECT_EQ(1u, fetched->version);

    ASSERT_EQ(1u, result.flags.size());
    EXPECT_EQ(1u, result.flags.count("flagA"));
    EXPECT_TRUE(result.segments.empty());
}

TEST(MemoryStoreApplyTest, ApplyPartial_UpsertsNewSegment) {
    MemoryStore store;
    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>(),
        std::unordered_map<std::string, SegmentDescriptor>(),
    });

    Segment seg_a;
    seg_a.version = 1;
    seg_a.key = "segA";

    auto result = store.Apply(FDv2ChangeSet{
        FDv2ChangeSet::Type::kPartial,
        std::vector<FDv2Change>{{"segA", SegmentDescriptor(seg_a)}},
        Selector{},
    });

    auto fetched = store.GetSegment("segA");
    ASSERT_TRUE(fetched);
    EXPECT_TRUE(fetched->item.has_value());
    EXPECT_EQ("segA", fetched->item->key);
    EXPECT_EQ(1u, fetched->version);

    EXPECT_TRUE(result.flags.empty());
    ASSERT_EQ(1u, result.segments.size());
    EXPECT_EQ(1u, result.segments.count("segA"));
}

TEST(MemoryStoreApplyTest, ApplyPartial_SkipsFlagWithLowerVersion) {
    MemoryStore store;
    Flag flag_a;
    flag_a.version = 5;
    flag_a.key = "flagA";

    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a)}},
        std::unordered_map<std::string, SegmentDescriptor>(),
    });

    Flag flag_a_stale;
    flag_a_stale.version = 3;
    flag_a_stale.key = "flagA";

    auto result = store.Apply(FDv2ChangeSet{
        FDv2ChangeSet::Type::kPartial,
        std::vector<FDv2Change>{{"flagA", FlagDescriptor(flag_a_stale)}},
        Selector{},
    });

    auto fetched = store.GetFlag("flagA");
    ASSERT_TRUE(fetched);
    EXPECT_EQ(5u, fetched->version);

    EXPECT_TRUE(result.flags.empty());
    EXPECT_TRUE(result.segments.empty());
}

TEST(MemoryStoreApplyTest, ApplyPartial_SkipsFlagWithEqualVersion) {
    MemoryStore store;
    Flag flag_a;
    flag_a.version = 5;
    flag_a.key = "flagA";

    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a)}},
        std::unordered_map<std::string, SegmentDescriptor>(),
    });

    Flag flag_a_same;
    flag_a_same.version = 5;
    flag_a_same.key = "flagA";

    auto result = store.Apply(FDv2ChangeSet{
        FDv2ChangeSet::Type::kPartial,
        std::vector<FDv2Change>{{"flagA", FlagDescriptor(flag_a_same)}},
        Selector{},
    });

    auto fetched = store.GetFlag("flagA");
    ASSERT_TRUE(fetched);
    EXPECT_EQ(5u, fetched->version);

    EXPECT_TRUE(result.flags.empty());
    EXPECT_TRUE(result.segments.empty());
}

TEST(MemoryStoreApplyTest, ApplyPartial_AppliesFlagWithHigherVersion) {
    MemoryStore store;
    Flag flag_a;
    flag_a.version = 5;
    flag_a.key = "flagA";

    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a)}},
        std::unordered_map<std::string, SegmentDescriptor>(),
    });

    Flag flag_a_new;
    flag_a_new.version = 6;
    flag_a_new.key = "flagA";

    auto result = store.Apply(FDv2ChangeSet{
        FDv2ChangeSet::Type::kPartial,
        std::vector<FDv2Change>{{"flagA", FlagDescriptor(flag_a_new)}},
        Selector{},
    });

    auto fetched = store.GetFlag("flagA");
    ASSERT_TRUE(fetched);
    EXPECT_EQ(6u, fetched->version);

    ASSERT_EQ(1u, result.flags.size());
    EXPECT_EQ(1u, result.flags.count("flagA"));
    EXPECT_TRUE(result.segments.empty());
}

TEST(MemoryStoreApplyTest, ApplyPartial_SkipsSegmentWithLowerVersion) {
    MemoryStore store;
    Segment seg_a;
    seg_a.version = 5;
    seg_a.key = "segA";

    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>(),
        std::unordered_map<std::string, SegmentDescriptor>{
            {"segA", SegmentDescriptor(seg_a)}},
    });

    Segment seg_a_stale;
    seg_a_stale.version = 3;
    seg_a_stale.key = "segA";

    auto result = store.Apply(FDv2ChangeSet{
        FDv2ChangeSet::Type::kPartial,
        std::vector<FDv2Change>{{"segA", SegmentDescriptor(seg_a_stale)}},
        Selector{},
    });

    auto fetched = store.GetSegment("segA");
    ASSERT_TRUE(fetched);
    EXPECT_EQ(5u, fetched->version);

    EXPECT_TRUE(result.flags.empty());
    EXPECT_TRUE(result.segments.empty());
}

TEST(MemoryStoreApplyTest, ApplyPartial_AppliesSegmentWithHigherVersion) {
    MemoryStore store;
    Segment seg_a;
    seg_a.version = 5;
    seg_a.key = "segA";

    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>(),
        std::unordered_map<std::string, SegmentDescriptor>{
            {"segA", SegmentDescriptor(seg_a)}},
    });

    Segment seg_a_new;
    seg_a_new.version = 6;
    seg_a_new.key = "segA";

    auto result = store.Apply(FDv2ChangeSet{
        FDv2ChangeSet::Type::kPartial,
        std::vector<FDv2Change>{{"segA", SegmentDescriptor(seg_a_new)}},
        Selector{},
    });

    auto fetched = store.GetSegment("segA");
    ASSERT_TRUE(fetched);
    EXPECT_EQ(6u, fetched->version);

    EXPECT_TRUE(result.flags.empty());
    ASSERT_EQ(1u, result.segments.size());
    EXPECT_EQ(1u, result.segments.count("segA"));
}

TEST(MemoryStoreApplyTest, ApplyPartial_PreservesUnchangedFlags) {
    MemoryStore store;
    Flag flag_a;
    flag_a.version = 1;
    flag_a.key = "flagA";

    Flag flag_b;
    flag_b.version = 1;
    flag_b.key = "flagB";

    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a)},
            {"flagB", FlagDescriptor(flag_b)}},
        std::unordered_map<std::string, SegmentDescriptor>(),
    });

    Flag flag_b_new;
    flag_b_new.version = 2;
    flag_b_new.key = "flagB";

    auto result = store.Apply(FDv2ChangeSet{
        FDv2ChangeSet::Type::kPartial,
        std::vector<FDv2Change>{{"flagB", FlagDescriptor(flag_b_new)}},
        Selector{},
    });

    auto fetched_a = store.GetFlag("flagA");
    ASSERT_TRUE(fetched_a);
    EXPECT_EQ(1u, fetched_a->version);

    auto fetched_b = store.GetFlag("flagB");
    ASSERT_TRUE(fetched_b);
    EXPECT_EQ(2u, fetched_b->version);

    ASSERT_EQ(1u, result.flags.size());
    EXPECT_EQ(1u, result.flags.count("flagB"));
    EXPECT_TRUE(result.segments.empty());
}

TEST(MemoryStoreApplyTest, ApplyPartial_PreservesUnchangedSegments) {
    MemoryStore store;
    Segment seg_a;
    seg_a.version = 1;
    seg_a.key = "segA";

    Segment seg_b;
    seg_b.version = 1;
    seg_b.key = "segB";

    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>(),
        std::unordered_map<std::string, SegmentDescriptor>{
            {"segA", SegmentDescriptor(seg_a)},
            {"segB", SegmentDescriptor(seg_b)}},
    });

    Segment seg_b_new;
    seg_b_new.version = 2;
    seg_b_new.key = "segB";

    auto result = store.Apply(FDv2ChangeSet{
        FDv2ChangeSet::Type::kPartial,
        std::vector<FDv2Change>{{"segB", SegmentDescriptor(seg_b_new)}},
        Selector{},
    });

    auto fetched_a = store.GetSegment("segA");
    ASSERT_TRUE(fetched_a);
    EXPECT_EQ(1u, fetched_a->version);

    auto fetched_b = store.GetSegment("segB");
    ASSERT_TRUE(fetched_b);
    EXPECT_EQ(2u, fetched_b->version);

    EXPECT_TRUE(result.flags.empty());
    ASSERT_EQ(1u, result.segments.size());
    EXPECT_EQ(1u, result.segments.count("segB"));
}

TEST(MemoryStoreApplyTest, ApplyPartial_WithFlagTombstone) {
    MemoryStore store;
    Flag flag_a;
    flag_a.version = 1;
    flag_a.key = "flagA";

    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a)}},
        std::unordered_map<std::string, SegmentDescriptor>(),
    });

    auto result = store.Apply(FDv2ChangeSet{
        FDv2ChangeSet::Type::kPartial,
        std::vector<FDv2Change>{{"flagA", FlagDescriptor(Tombstone(2))}},
        Selector{},
    });

    auto fetched = store.GetFlag("flagA");
    ASSERT_TRUE(fetched);
    EXPECT_EQ(2u, fetched->version);
    EXPECT_FALSE(fetched->item.has_value());

    ASSERT_EQ(1u, result.flags.size());
    EXPECT_EQ(1u, result.flags.count("flagA"));
    EXPECT_TRUE(result.segments.empty());
}

TEST(MemoryStoreApplyTest, ApplyPartial_TombstoneSkippedIfVersionNotNewer) {
    MemoryStore store;
    Flag flag_a;
    flag_a.version = 5;
    flag_a.key = "flagA";

    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a)}},
        std::unordered_map<std::string, SegmentDescriptor>(),
    });

    // Tombstone at version 3 < stored version 5: should be ignored.
    auto result = store.Apply(FDv2ChangeSet{
        FDv2ChangeSet::Type::kPartial,
        std::vector<FDv2Change>{{"flagA", FlagDescriptor(Tombstone(3))}},
        Selector{},
    });

    auto fetched = store.GetFlag("flagA");
    ASSERT_TRUE(fetched);
    EXPECT_EQ(5u, fetched->version);
    EXPECT_TRUE(fetched->item.has_value());

    EXPECT_TRUE(result.flags.empty());
    EXPECT_TRUE(result.segments.empty());
}

TEST(MemoryStoreApplyTest, ApplyPartial_MixedStaleAndFreshItems) {
    MemoryStore store;
    Flag flag_a;
    flag_a.version = 10;
    flag_a.key = "flagA";

    Flag flag_b;
    flag_b.version = 1;
    flag_b.key = "flagB";

    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a)},
            {"flagB", FlagDescriptor(flag_b)}},
        std::unordered_map<std::string, SegmentDescriptor>(),
    });

    Flag flag_a_stale;
    flag_a_stale.version = 5;
    flag_a_stale.key = "flagA";

    Flag flag_b_new;
    flag_b_new.version = 2;
    flag_b_new.key = "flagB";

    auto result = store.Apply(FDv2ChangeSet{
        FDv2ChangeSet::Type::kPartial,
        std::vector<FDv2Change>{{"flagA", FlagDescriptor(flag_a_stale)},
                                {"flagB", FlagDescriptor(flag_b_new)}},
        Selector{},
    });

    // flagA version 5 < 10: skip.
    EXPECT_EQ(10u, store.GetFlag("flagA")->version);
    // flagB version 2 > 1: apply.
    EXPECT_EQ(2u, store.GetFlag("flagB")->version);

    ASSERT_EQ(1u, result.flags.size());
    EXPECT_EQ(1u, result.flags.count("flagB"));
    EXPECT_TRUE(result.segments.empty());
}
