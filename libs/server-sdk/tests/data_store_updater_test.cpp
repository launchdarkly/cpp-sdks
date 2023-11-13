#include <gtest/gtest.h>

#include <data_components/change_notifier/change_notifier_destination.hpp>
#include <data_components/memory_store/memory_store.hpp>
#include <launchdarkly/data_model/descriptors.hpp>

using launchdarkly::Value;

using namespace launchdarkly::data_model;
using namespace launchdarkly::server_side::data_components;

TEST(ChangeNotifierDestinationTest, DoesNotInitializeStoreUntilInit) {
    MemoryStore store;
    ChangeNotifierDestination updater(store, store);
    EXPECT_FALSE(store.Initialized());
}

TEST(ChangeNotifierDestinationTest, InitializesStore) {
    MemoryStore store;
    ChangeNotifierDestination updater(store, store);
    updater.Init(SDKDataSet());
    EXPECT_TRUE(store.Initialized());
}

TEST(ChangeNotifierDestinationTest, InitPropagatesData) {
    MemoryStore store;
    ChangeNotifierDestination updater(store, store);
    Flag flag;
    flag.version = 1;
    flag.key = "flagA";
    flag.on = true;
    flag.variations = std::vector<Value>{true, false};
    Flag::Variation variation = 0;
    flag.fallthrough = variation;

    auto segment = Segment();
    segment.version = 1;
    segment.key = "segmentA";

    updater.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag)}},
        std::unordered_map<std::string, SegmentDescriptor>{
            {"segmentA", SegmentDescriptor(segment)}},
    });

    auto fetched_flag = store.GetFlag("flagA");
    EXPECT_TRUE(fetched_flag);
    EXPECT_TRUE(fetched_flag->item);
    EXPECT_EQ("flagA", fetched_flag->item->key);
    EXPECT_EQ(1, fetched_flag->item->version);
    EXPECT_EQ(fetched_flag->version, fetched_flag->item->version);

    auto fetched_segment = store.GetSegment("segmentA");
    EXPECT_TRUE(fetched_segment);
    EXPECT_TRUE(fetched_segment->item);
    EXPECT_EQ("segmentA", fetched_segment->item->key);
    EXPECT_EQ(1, fetched_segment->item->version);
    EXPECT_EQ(fetched_segment->version, fetched_segment->item->version);
}

TEST(ChangeNotifierDestinationTest, SecondInitProducesChanges) {
    MemoryStore store;
    ChangeNotifierDestination updater(store, store);
    Flag flag_a_v1;
    flag_a_v1.version = 1;
    flag_a_v1.key = "flagA";
    flag_a_v1.on = true;
    flag_a_v1.variations = std::vector<Value>{true, false};
    Flag::Variation variation = 0;
    flag_a_v1.fallthrough = variation;

    Flag flag_b_v1;
    flag_b_v1.version = 1;
    flag_b_v1.key = "flagA";
    flag_b_v1.on = true;
    flag_b_v1.variations = std::vector<Value>{true, false};
    flag_b_v1.fallthrough = variation;

    Flag flab_c_v1;
    flab_c_v1.version = 1;
    flab_c_v1.key = "flagA";
    flab_c_v1.on = true;
    flab_c_v1.variations = std::vector<Value>{true, false};
    flab_c_v1.fallthrough = variation;

    updater.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a_v1)},
            {"flagB", FlagDescriptor(flag_b_v1)}},
        std::unordered_map<std::string, SegmentDescriptor>(),
    });

    Flag flag_a_v2;
    flag_a_v2.version = 2;
    flag_a_v2.key = "flagA";
    flag_a_v2.on = true;
    flag_a_v2.variations = std::vector<Value>{true, false};
    flag_a_v2.fallthrough = variation;

    // Not updated.
    Flag flag_c_v1_second;
    flag_c_v1_second.version = 1;
    flag_c_v1_second.key = "flagC";
    flag_c_v1_second.on = true;
    flag_c_v1_second.variations = std::vector<Value>{true, false};
    flag_c_v1_second.fallthrough = variation;

    // New flag
    Flag flag_d;
    flag_d.version = 2;
    flag_d.key = "flagD";
    flag_d.on = true;
    flag_d.variations = std::vector<Value>{true, false};
    flag_d.fallthrough = variation;

    std::atomic<bool> got_event(false);
    updater.OnFlagChange(
        [&got_event](std::shared_ptr<std::set<std::string>> changeset) {
            got_event = true;
            std::vector<std::string> diff;
            auto expectedSet = std::set<std::string>{"flagA", "flagB", "flagD"};
            std::set_difference(expectedSet.begin(), expectedSet.end(),
                                changeset->begin(), changeset->end(),
                                std::inserter(diff, diff.begin()));
            EXPECT_EQ(0, diff.size());
        });

    // Updated flag A, deleted flag B, added flag C.
    updater.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a_v2)},
            {"flagD", FlagDescriptor(flag_d)},
            {"flagC", FlagDescriptor(flag_c_v1_second)}},
        std::unordered_map<std::string, SegmentDescriptor>(),
    });

    EXPECT_TRUE(got_event);
}

TEST(ChangeNotifierDestinationTest, CanUpsertNewFlag) {
    MemoryStore store;
    ChangeNotifierDestination updater(store, store);

    Flag flag_a;
    flag_a.version = 1;
    flag_a.key = "flagA";

    updater.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>(),
        std::unordered_map<std::string, SegmentDescriptor>(),
    });
    updater.Upsert("flagA", FlagDescriptor(flag_a));

    auto fetched_flag = store.GetFlag("flagA");
    EXPECT_TRUE(fetched_flag);
    EXPECT_TRUE(fetched_flag->item);
    EXPECT_EQ("flagA", fetched_flag->item->key);
    EXPECT_EQ(1, fetched_flag->item->version);
    EXPECT_EQ(fetched_flag->version, fetched_flag->item->version);
}

TEST(ChangeNotifierDestinationTest, CanUpsertExitingFlag) {
    Flag flag_a;
    flag_a.version = 1;
    flag_a.key = "flagA";

    MemoryStore store;
    ChangeNotifierDestination updater(store, store);

    updater.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a)}},
        std::unordered_map<std::string, SegmentDescriptor>(),
    });

    Flag flag_a_2;
    flag_a_2.version = 2;
    flag_a_2.key = "flagA";

    updater.Upsert("flagA", FlagDescriptor(flag_a_2));

    auto fetched_flag = store.GetFlag("flagA");
    EXPECT_TRUE(fetched_flag);
    EXPECT_TRUE(fetched_flag->item);
    EXPECT_EQ("flagA", fetched_flag->item->key);
    EXPECT_EQ(2, fetched_flag->item->version);
    EXPECT_EQ(fetched_flag->version, fetched_flag->item->version);
}

TEST(ChangeNotifierDestinationTest, OldVersionIsDiscardedOnUpsertFlag) {
    Flag flag_a;
    flag_a.version = 2;
    flag_a.key = "flagA";
    flag_a.variations = std::vector<Value>{"potato", "ham"};

    MemoryStore store;
    ChangeNotifierDestination updater(store, store);

    updater.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a)}},
        std::unordered_map<std::string, SegmentDescriptor>(),
    });

    Flag flag_a_2;
    flag_a_2.version = 1;
    flag_a_2.key = "flagA";
    flag_a.variations = std::vector<Value>{"potato"};

    updater.Upsert("flagA", FlagDescriptor(flag_a_2));

    auto fetched_flag = store.GetFlag("flagA");
    EXPECT_TRUE(fetched_flag);
    EXPECT_TRUE(fetched_flag->item);
    EXPECT_EQ("flagA", fetched_flag->item->key);
    EXPECT_EQ(2, fetched_flag->item->version);
    EXPECT_EQ(fetched_flag->version, fetched_flag->item->version);
    EXPECT_EQ(2, fetched_flag->item->variations.size());
    EXPECT_EQ(std::string("potato"),
              fetched_flag->item->variations[0].AsString());
    EXPECT_EQ(std::string("ham"), fetched_flag->item->variations[1].AsString());
}

TEST(ChangeNotifierDestinationTest, CanUpsertNewSegment) {
    Segment segment_a;
    segment_a.version = 1;
    segment_a.key = "segmentA";

    MemoryStore store;
    ChangeNotifierDestination updater(store, store);

    updater.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>(),
        std::unordered_map<std::string, SegmentDescriptor>(),
    });
    updater.Upsert("segmentA", SegmentDescriptor(segment_a));

    auto fetched_segment = store.GetSegment("segmentA");
    EXPECT_TRUE(fetched_segment);
    EXPECT_TRUE(fetched_segment->item);
    EXPECT_EQ("segmentA", fetched_segment->item->key);
    EXPECT_EQ(1, fetched_segment->item->version);
    EXPECT_EQ(fetched_segment->version, fetched_segment->item->version);
}

TEST(ChangeNotifierDestinationTest, CanUpsertExitingSegment) {
    Segment segment_a;
    segment_a.version = 1;
    segment_a.key = "segmentA";

    MemoryStore store;
    ChangeNotifierDestination updater(store, store);

    updater.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>(),
        std::unordered_map<std::string, SegmentDescriptor>{
            {"segmentA", SegmentDescriptor(segment_a)}},
    });

    Segment segment_a_2;
    segment_a_2.version = 2;
    segment_a_2.key = "segmentA";

    updater.Upsert("segmentA", SegmentDescriptor(segment_a_2));

    auto fetched_segment = store.GetSegment("segmentA");
    EXPECT_TRUE(fetched_segment);
    EXPECT_TRUE(fetched_segment->item);
    EXPECT_EQ("segmentA", fetched_segment->item->key);
    EXPECT_EQ(2, fetched_segment->item->version);
    EXPECT_EQ(fetched_segment->version, fetched_segment->item->version);
}

TEST(ChangeNotifierDestinationTest, OldVersionIsDiscardedOnUpsertSegment) {
    Segment segment_a;
    segment_a.version = 2;
    segment_a.key = "segmentA";

    MemoryStore store;
    ChangeNotifierDestination updater(store, store);

    updater.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>(),
        std::unordered_map<std::string, SegmentDescriptor>{
            {"segmentA", SegmentDescriptor(segment_a)}},
    });

    Segment segment_a_2;
    segment_a_2.version = 1;
    segment_a_2.key = "segmentA";

    updater.Upsert("segmentA", SegmentDescriptor(segment_a_2));

    auto fetched_segment = store.GetSegment("segmentA");
    EXPECT_TRUE(fetched_segment);
    EXPECT_TRUE(fetched_segment->item);
    EXPECT_EQ("segmentA", fetched_segment->item->key);
    EXPECT_EQ(2, fetched_segment->item->version);
    EXPECT_EQ(fetched_segment->version, fetched_segment->item->version);
}

TEST(ChangeNotifierDestinationTest, ProducesChangeEventsOnUpsert) {
    Flag flag_a;
    Flag flag_b;

    flag_a.key = "flagA";
    flag_a.version = 1;

    flag_b.key = "flagB";
    flag_b.version = 1;

    flag_b.prerequisites.push_back(Flag::Prerequisite{"flagA", 0});

    MemoryStore store;
    ChangeNotifierDestination updater(store, store);

    updater.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a)},
            {"flagB", FlagDescriptor(flag_b)}},
        std::unordered_map<std::string, SegmentDescriptor>(),
    });

    Flag flag_a_2;
    flag_a_2.key = "flagA";
    flag_a_2.version = 2;

    std::atomic<bool> got_event(false);
    updater.OnFlagChange(
        [&got_event](std::shared_ptr<std::set<std::string>> changeset) {
            got_event = true;
            std::vector<std::string> diff;
            auto expectedSet = std::set<std::string>{"flagA", "flagB"};
            std::set_difference(expectedSet.begin(), expectedSet.end(),
                                changeset->begin(), changeset->end(),
                                std::inserter(diff, diff.begin()));
            EXPECT_EQ(0, diff.size());
        });

    updater.Upsert("flagA", FlagDescriptor(flag_a_2));

    EXPECT_EQ(true, got_event);
}

TEST(ChangeNotifierDestinationTest, ProducesNoEventIfNoFlagChanged) {
    Flag flag_a;
    Flag flag_b;

    flag_a.key = "flagA";
    flag_a.version = 1;

    flag_b.key = "flagB";
    flag_b.version = 1;

    flag_b.prerequisites.push_back(Flag::Prerequisite{"flagA", 0});

    MemoryStore store;
    ChangeNotifierDestination updater(store, store);

    Segment segment_a;
    segment_a.version = 1;
    segment_a.key = "segmentA";

    updater.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a)},
            {"flagB", FlagDescriptor(flag_b)}},
        std::unordered_map<std::string, SegmentDescriptor>{
            {"segmentA", SegmentDescriptor(segment_a)},
        },
    });

    Segment segment_a_2;
    segment_a_2.key = "flagA";
    segment_a_2.version = 2;

    std::atomic<bool> got_event(false);
    updater.OnFlagChange(
        [&got_event](std::shared_ptr<std::set<std::string>> changeset) {
            got_event = true;
        });

    updater.Upsert("segmentA", SegmentDescriptor(segment_a_2));

    EXPECT_EQ(false, got_event);
}

TEST(ChangeNotifierDestinationTest, NoEventOnDiscardedUpsert) {
    Flag flag_a;
    Flag flag_b;

    flag_a.key = "flagA";
    flag_a.version = 1;

    flag_b.key = "flagB";
    flag_b.version = 1;

    flag_b.prerequisites.push_back(Flag::Prerequisite{"flagA", 0});

    MemoryStore store;
    ChangeNotifierDestination updater(store, store);

    updater.Init(SDKDataSet{
        std::unordered_map<std::string, FlagDescriptor>{
            {"flagA", FlagDescriptor(flag_a)},
            {"flagB", FlagDescriptor(flag_b)}},
        std::unordered_map<std::string, SegmentDescriptor>(),
    });

    Flag flag_a_2;
    flag_a_2.key = "flagA";
    flag_a_2.version = 1;

    std::atomic<bool> got_event(false);
    updater.OnFlagChange(
        [&got_event](std::shared_ptr<std::set<std::string>> changeset) {
            got_event = true;
        });

    updater.Upsert("flagA", FlagDescriptor(flag_a_2));

    EXPECT_EQ(false, got_event);
}
