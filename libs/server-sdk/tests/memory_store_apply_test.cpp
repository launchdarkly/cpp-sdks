#include <gtest/gtest.h>

#include <data_components/memory_store/memory_store.hpp>
#include <data_interfaces/item_change.hpp>

#include <launchdarkly/data_model/change_set.hpp>
#include <launchdarkly/data_model/fdv2_change.hpp>

using namespace launchdarkly::data_model;
using namespace launchdarkly::server_side::data_components;
using namespace launchdarkly::server_side::data_interfaces;

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

    store.Apply(ChangeSet<ChangeSetData>{ChangeSetType::kNone, {}, Selector{}});

    auto fetched_flag = store.GetFlag("flagA");
    ASSERT_TRUE(fetched_flag);
    EXPECT_EQ(1u, fetched_flag->version);
    auto fetched_seg = store.GetSegment("segA");
    ASSERT_TRUE(fetched_seg);
    EXPECT_EQ(1u, fetched_seg->version);
}

TEST(MemoryStoreApplyTest, ApplyNone_DoesNotInitialize) {
    MemoryStore store;
    store.Apply(ChangeSet<ChangeSetData>{ChangeSetType::kNone, {}, Selector{}});
    EXPECT_FALSE(store.Initialized());
}

// ---------------------------------------------------------------------------
// kFull tests
// ---------------------------------------------------------------------------

TEST(MemoryStoreApplyTest, ApplyFull_SetsInitialized) {
    MemoryStore store;
    ASSERT_FALSE(store.Initialized());
    store.Apply(ChangeSet<ChangeSetData>{ChangeSetType::kFull, {}, Selector{}});
    EXPECT_TRUE(store.Initialized());
}

TEST(MemoryStoreApplyTest, ApplyFull_StoresItems) {
    MemoryStore store;
    Flag flag_a;
    flag_a.version = 1;
    flag_a.key = "flagA";

    Segment seg_a;
    seg_a.version = 1;
    seg_a.key = "segA";

    store.Apply(ChangeSet<ChangeSetData>{
        ChangeSetType::kFull,
        ChangeSetData{ItemChange{"flagA", FlagDescriptor(flag_a)},
                      ItemChange{"segA", SegmentDescriptor(seg_a)}},
        Selector{},
    });

    auto fetched_flag = store.GetFlag("flagA");
    ASSERT_TRUE(fetched_flag);
    EXPECT_TRUE(fetched_flag->item.has_value());
    EXPECT_EQ("flagA", fetched_flag->item->key);
    EXPECT_EQ(1u, fetched_flag->version);

    auto fetched_seg = store.GetSegment("segA");
    ASSERT_TRUE(fetched_seg);
    EXPECT_TRUE(fetched_seg->item.has_value());
    EXPECT_EQ("segA", fetched_seg->item->key);
    EXPECT_EQ(1u, fetched_seg->version);
}

TEST(MemoryStoreApplyTest, ApplyFull_ClearsExistingItems) {
    MemoryStore store;
    Flag flag_a;
    flag_a.version = 1;
    flag_a.key = "flagA";

    Flag flag_b;
    flag_b.version = 1;
    flag_b.key = "flagB";

    Segment seg_a;
    seg_a.version = 1;
    seg_a.key = "segA";

    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a)},
            {"flagB", FlagDescriptor(flag_b)}},
        std::unordered_map<std::string, SegmentDescriptor>{
            {"segA", SegmentDescriptor(seg_a)}},
    });

    Flag flag_c;
    flag_c.version = 1;
    flag_c.key = "flagC";

    Segment seg_b;
    seg_b.version = 1;
    seg_b.key = "segB";

    store.Apply(ChangeSet<ChangeSetData>{
        ChangeSetType::kFull,
        ChangeSetData{ItemChange{"flagC", FlagDescriptor(flag_c)},
                      ItemChange{"segB", SegmentDescriptor(seg_b)}},
        Selector{},
    });

    EXPECT_FALSE(store.GetFlag("flagA"));
    EXPECT_FALSE(store.GetFlag("flagB"));
    ASSERT_TRUE(store.GetFlag("flagC"));
    EXPECT_FALSE(store.GetSegment("segA"));
    ASSERT_TRUE(store.GetSegment("segB"));
}

// ---------------------------------------------------------------------------
// kPartial tests
// ---------------------------------------------------------------------------

TEST(MemoryStoreApplyTest, ApplyPartial_AppliesItems) {
    MemoryStore store;
    Flag flag_a;
    flag_a.version = 5;
    flag_a.key = "flagA";

    Segment seg_a;
    seg_a.version = 5;
    seg_a.key = "segA";

    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a)}},
        std::unordered_map<std::string, SegmentDescriptor>{
            {"segA", SegmentDescriptor(seg_a)}},
    });

    Flag flag_a_new;
    flag_a_new.version = 6;
    flag_a_new.key = "flagA";

    Segment seg_a_new;
    seg_a_new.version = 6;
    seg_a_new.key = "segA";

    store.Apply(ChangeSet<ChangeSetData>{
        ChangeSetType::kPartial,
        ChangeSetData{ItemChange{"flagA", FlagDescriptor(flag_a_new)},
                      ItemChange{"segA", SegmentDescriptor(seg_a_new)}},
        Selector{},
    });

    ASSERT_TRUE(store.GetFlag("flagA"));
    EXPECT_EQ(6u, store.GetFlag("flagA")->version);
    ASSERT_TRUE(store.GetSegment("segA"));
    EXPECT_EQ(6u, store.GetSegment("segA")->version);
}

TEST(MemoryStoreApplyTest, ApplyPartial_PreservesUnchangedItems) {
    MemoryStore store;
    Flag flag_a;
    flag_a.version = 1;
    flag_a.key = "flagA";

    Flag flag_b;
    flag_b.version = 1;
    flag_b.key = "flagB";

    Segment seg_a;
    seg_a.version = 1;
    seg_a.key = "segA";

    Segment seg_b;
    seg_b.version = 1;
    seg_b.key = "segB";

    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a)},
            {"flagB", FlagDescriptor(flag_b)}},
        std::unordered_map<std::string, SegmentDescriptor>{
            {"segA", SegmentDescriptor(seg_a)},
            {"segB", SegmentDescriptor(seg_b)}},
    });

    Flag flag_b_new;
    flag_b_new.version = 2;
    flag_b_new.key = "flagB";

    Segment seg_b_new;
    seg_b_new.version = 2;
    seg_b_new.key = "segB";

    store.Apply(ChangeSet<ChangeSetData>{
        ChangeSetType::kPartial,
        ChangeSetData{ItemChange{"flagB", FlagDescriptor(flag_b_new)},
                      ItemChange{"segB", SegmentDescriptor(seg_b_new)}},
        Selector{},
    });

    ASSERT_TRUE(store.GetFlag("flagA"));
    EXPECT_EQ(1u, store.GetFlag("flagA")->version);
    ASSERT_TRUE(store.GetFlag("flagB"));
    EXPECT_EQ(2u, store.GetFlag("flagB")->version);
    ASSERT_TRUE(store.GetSegment("segA"));
    EXPECT_EQ(1u, store.GetSegment("segA")->version);
    ASSERT_TRUE(store.GetSegment("segB"));
    EXPECT_EQ(2u, store.GetSegment("segB")->version);
}
