#include <gtest/gtest.h>

#include <data_components/memory_store/memory_store.hpp>

using launchdarkly::Value;

using namespace launchdarkly::data_model;
using namespace launchdarkly::server_side::data_components;

TEST(MemoryStoreTest, StartsUninitialized) {
    MemoryStore store;
    EXPECT_FALSE(store.Initialized());
}

TEST(MemoryStoreTest, IsInitializedAfterInit) {
    MemoryStore store;
    store.Init(SDKDataSet());
    EXPECT_TRUE(store.Initialized());
}

TEST(MemoryStoreTest, HasDescription) {
    MemoryStore store;
    EXPECT_EQ(std::string("memory"), store.Identity());
}

TEST(MemoryStoreTest, CanGetFlag) {
    MemoryStore store;
    Flag flag;
    flag.version = 1;
    flag.key = "flagA";
    flag.on = true;
    flag.variations = std::vector<Value>{true, false};
    Flag::Variation variation = 0;
    flag.fallthrough = variation;
    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag)}},
        std::unordered_map<std::string, SegmentDescriptor>(),
    });

    auto fetched_flag = store.GetFlag("flagA");
    EXPECT_TRUE(fetched_flag);
    EXPECT_TRUE(fetched_flag->item);
    EXPECT_EQ("flagA", fetched_flag->item->key);
    EXPECT_EQ(1, fetched_flag->item->version);
    EXPECT_EQ(fetched_flag->version, fetched_flag->item->version);
}

TEST(MemoryStoreTest, CanGetAllFlags) {
    Flag flag_a;
    flag_a.version = 1;
    flag_a.key = "flagA";

    Flag flag_b;
    flag_b.version = 2;
    flag_b.key = "flagB";

    MemoryStore store;
    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a)},
            {"flagB", FlagDescriptor(flag_b)}},
        std::unordered_map<std::string, SegmentDescriptor>(),
    });

    auto fetched = store.AllFlags();
    EXPECT_EQ(2, fetched.size());

    EXPECT_EQ(std::string("flagA"), fetched["flagA"]->item->key);
    EXPECT_EQ(std::string("flagB"), fetched["flagB"]->item->key);
}

TEST(MemoryStoreTest, CanGetAllFlagsWhenThereAreNoFlags) {
    MemoryStore store;
    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>(),
        std::unordered_map<std::string, SegmentDescriptor>(),
    });

    auto fetched = store.AllFlags();
    EXPECT_EQ(0, fetched.size());
}

TEST(MemoryStoreTest, CanGetSegment) {
    MemoryStore store;
    auto segment = Segment();
    segment.version = 1;
    segment.key = "segmentA";
    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>(),
        std::unordered_map<std::string, SegmentDescriptor>{
            {"segmentA", SegmentDescriptor(segment)}},
    });

    auto fetched_segment = store.GetSegment("segmentA");
    EXPECT_TRUE(fetched_segment);
    EXPECT_TRUE(fetched_segment->item);
    EXPECT_EQ("segmentA", fetched_segment->item->key);
    EXPECT_EQ(1, fetched_segment->item->version);
    EXPECT_EQ(fetched_segment->version, fetched_segment->item->version);
}

TEST(MemoryStoreTest, CanGetAllSegments) {
    auto segment_a = Segment();
    segment_a.version = 1;
    segment_a.key = "segmentA";

    auto segment_b = Segment();
    segment_b.version = 2;
    segment_b.key = "segmentB";

    MemoryStore store;
    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>(),
        std::unordered_map<std::string, SegmentDescriptor>{
            {"segmentA", SegmentDescriptor(segment_a)},
            {"segmentB", SegmentDescriptor(segment_b)}},
    });

    auto fetched = store.AllSegments();
    EXPECT_EQ(2, fetched.size());

    EXPECT_EQ(std::string("segmentA"), fetched["segmentA"]->item->key);
    EXPECT_EQ(std::string("segmentB"), fetched["segmentB"]->item->key);
}

TEST(MemoryStoreTest, CanGetAllSegmentsWhenThereAreNoSegments) {
    MemoryStore store;
    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>(),
        std::unordered_map<std::string, SegmentDescriptor>(),
    });

    auto fetched = store.AllSegments();
    EXPECT_EQ(0, fetched.size());
}

TEST(MemoryStoreTest, GetMissingFlagOrSegment) {
    MemoryStore store;
    auto fetched_flag = store.GetFlag("flagA");
    EXPECT_FALSE(fetched_flag);
    auto fetched_segment = store.GetSegment("segmentA");
    EXPECT_FALSE(fetched_segment);
}

TEST(MemoryStoreTest, CanUpsertNewFlag) {
    Flag flag_a;
    flag_a.version = 1;
    flag_a.key = "flagA";

    MemoryStore store;
    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>(),
        std::unordered_map<std::string, SegmentDescriptor>(),
    });
    store.Upsert("flagA", FlagDescriptor(flag_a));

    auto fetched_flag = store.GetFlag("flagA");
    EXPECT_TRUE(fetched_flag);
    EXPECT_TRUE(fetched_flag->item);
    EXPECT_EQ("flagA", fetched_flag->item->key);
    EXPECT_EQ(1, fetched_flag->item->version);
    EXPECT_EQ(fetched_flag->version, fetched_flag->item->version);
}

TEST(MemoryStoreTest, CanUpsertExitingFlag) {
    Flag flag_a;
    flag_a.version = 1;
    flag_a.key = "flagA";

    MemoryStore store;
    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a)}},
        std::unordered_map<std::string, SegmentDescriptor>(),
    });

    Flag flag_a_2;
    flag_a_2.version = 2;
    flag_a_2.key = "flagA";

    store.Upsert("flagA", FlagDescriptor(flag_a_2));

    auto fetched_flag = store.GetFlag("flagA");
    EXPECT_TRUE(fetched_flag);
    EXPECT_TRUE(fetched_flag->item);
    EXPECT_EQ("flagA", fetched_flag->item->key);
    EXPECT_EQ(2, fetched_flag->item->version);
    EXPECT_EQ(fetched_flag->version, fetched_flag->item->version);
}

TEST(MemoryStoreTest, CanUpsertNewSegment) {
    Segment segment_a;
    segment_a.version = 1;
    segment_a.key = "segmentA";

    MemoryStore store;
    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>(),
        std::unordered_map<std::string, SegmentDescriptor>(),
    });
    store.Upsert("segmentA", SegmentDescriptor(segment_a));

    auto fetched_segment = store.GetSegment("segmentA");
    EXPECT_TRUE(fetched_segment);
    EXPECT_TRUE(fetched_segment->item);
    EXPECT_EQ("segmentA", fetched_segment->item->key);
    EXPECT_EQ(1, fetched_segment->item->version);
    EXPECT_EQ(fetched_segment->version, fetched_segment->item->version);
}

TEST(MemoryStoreTest, CanUpsertExitingSegment) {
    Segment segment_a;
    segment_a.version = 1;
    segment_a.key = "segmentA";

    MemoryStore store;
    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>(),
        std::unordered_map<std::string, SegmentDescriptor>{
            {"segmentA", SegmentDescriptor(segment_a)}},
    });

    Segment segment_a_2;
    segment_a_2.version = 2;
    segment_a_2.key = "segmentA";

    store.Upsert("segmentA", SegmentDescriptor(segment_a_2));

    auto fetched_segment = store.GetSegment("segmentA");
    EXPECT_TRUE(fetched_segment);
    EXPECT_TRUE(fetched_segment->item);
    EXPECT_EQ("segmentA", fetched_segment->item->key);
    EXPECT_EQ(2, fetched_segment->item->version);
    EXPECT_EQ(fetched_segment->version, fetched_segment->item->version);
}

TEST(MemoryStoreTest, OriginalFlagValidAfterUpsertOfFlag) {
    Flag flag_a;
    flag_a.version = 1;
    flag_a.key = "flagA";
    flag_a.variations = std::vector<Value>{"potato", "ham"};

    MemoryStore store;
    store.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a)}},
        std::unordered_map<std::string, SegmentDescriptor>(),
    });
    auto fetched_flag_before = store.GetFlag("flagA");

    Flag flag_a_2;
    flag_a_2.version = 2;
    flag_a_2.key = "flagA";
    flag_a_2.variations = std::vector<Value>{"potato"};

    store.Upsert("flagA", FlagDescriptor(flag_a_2));

    auto fetched_flag_after = store.GetFlag("flagA");

    EXPECT_TRUE(fetched_flag_before);
    EXPECT_TRUE(fetched_flag_before->item);
    EXPECT_EQ("flagA", fetched_flag_before->item->key);
    EXPECT_EQ(1, fetched_flag_before->item->version);
    EXPECT_EQ(fetched_flag_before->version, fetched_flag_before->item->version);
    EXPECT_EQ(2, fetched_flag_before->item->variations.size());
    EXPECT_EQ(std::string("potato"),
              fetched_flag_before->item->variations[0].AsString());
    EXPECT_EQ(std::string("ham"),
              fetched_flag_before->item->variations[1].AsString());

    EXPECT_TRUE(fetched_flag_after);
    EXPECT_TRUE(fetched_flag_after->item);
    EXPECT_EQ("flagA", fetched_flag_after->item->key);
    EXPECT_EQ(2, fetched_flag_after->item->version);
    EXPECT_EQ(fetched_flag_after->version, fetched_flag_after->item->version);
    EXPECT_EQ(1, fetched_flag_after->item->variations.size());
    EXPECT_EQ(std::string("potato"),
              fetched_flag_after->item->variations[0].AsString());
}
