#include <gtest/gtest.h>

#include "data_store/data_store_updater.hpp"
#include "data_store/memory_store.hpp"

using launchdarkly::data_model::SDKDataSet;
using launchdarkly::server_side::data_store::DataStoreUpdater;
using launchdarkly::server_side::data_store::IDataStore;
using launchdarkly::server_side::data_store::MemoryStore;

using launchdarkly::Value;
using launchdarkly::data_model::Flag;
using launchdarkly::data_model::Segment;

TEST(DataStoreUpdaterTest, DoesNotInitializeStoreUntilInit) {
    auto store = std::make_shared<MemoryStore>();
    DataStoreUpdater updater(store, store);
    EXPECT_FALSE(store->Initialized());
}

TEST(DataStoreUpdaterTest, InitializesStore) {
    auto store = std::make_shared<MemoryStore>();
    DataStoreUpdater updater(store, store);
    updater.Init(SDKDataSet());
    EXPECT_TRUE(store->Initialized());
}

TEST(DataStoreUpdaterTest, InitPropagatesData) {
    auto store = std::make_shared<MemoryStore>();
    DataStoreUpdater updater(store, store);
    Flag flag;
    flag.version = 1;
    flag.key = "flagA";
    flag.on = true;
    flag.variations = std::vector<Value>{true, false};
    flag.fallthrough = 0;

    auto segment = Segment();
    segment.version = 1;
    segment.key = "segmentA";

    updater.Init(SDKDataSet{
        std::unordered_map<std::string, IDataStore::FlagDescriptor>{
            {"flagA", IDataStore::FlagDescriptor(flag)}},
        std::unordered_map<std::string, IDataStore::SegmentDescriptor>{
            {"segmentA", IDataStore::SegmentDescriptor(segment)}},
    });

    auto fetched_flag = store->GetFlag("flagA");
    EXPECT_TRUE(fetched_flag);
    EXPECT_TRUE(fetched_flag->item);
    EXPECT_EQ("flagA", fetched_flag->item->key);
    EXPECT_EQ(1, fetched_flag->item->version);
    EXPECT_EQ(fetched_flag->version, fetched_flag->item->version);

    auto fetched_segment = store->GetSegment("segmentA");
    EXPECT_TRUE(fetched_segment);
    EXPECT_TRUE(fetched_segment->item);
    EXPECT_EQ("segmentA", fetched_segment->item->key);
    EXPECT_EQ(1, fetched_segment->item->version);
    EXPECT_EQ(fetched_segment->version, fetched_segment->item->version);
}

TEST(DataStoreUpdaterTest, CanUpsertNewFlag) {
    auto store = std::make_shared<MemoryStore>();
    DataStoreUpdater updater(store, store);

    Flag flag_a;
    flag_a.version = 1;
    flag_a.key = "flagA";

    updater.Init(SDKDataSet{
        std::unordered_map<std::string, IDataStore::FlagDescriptor>(),
        std::unordered_map<std::string, IDataStore::SegmentDescriptor>(),
    });
    updater.Upsert("flagA", IDataStore::FlagDescriptor(flag_a));

    auto fetched_flag = store->GetFlag("flagA");
    EXPECT_TRUE(fetched_flag);
    EXPECT_TRUE(fetched_flag->item);
    EXPECT_EQ("flagA", fetched_flag->item->key);
    EXPECT_EQ(1, fetched_flag->item->version);
    EXPECT_EQ(fetched_flag->version, fetched_flag->item->version);
}

TEST(DataStoreUpdaterTest, CanUpsertExitingFlag) {
    Flag flag_a;
    flag_a.version = 1;
    flag_a.key = "flagA";

    auto store = std::make_shared<MemoryStore>();
    DataStoreUpdater updater(store, store);

    updater.Init(SDKDataSet{
        std::unordered_map<std::string, IDataStore::FlagDescriptor>{
            {"flagA", IDataStore::FlagDescriptor(flag_a)}},
        std::unordered_map<std::string, IDataStore::SegmentDescriptor>(),
    });

    Flag flag_a_2;
    flag_a_2.version = 2;
    flag_a_2.key = "flagA";

    updater.Upsert("flagA", IDataStore::FlagDescriptor(flag_a_2));

    auto fetched_flag = store->GetFlag("flagA");
    EXPECT_TRUE(fetched_flag);
    EXPECT_TRUE(fetched_flag->item);
    EXPECT_EQ("flagA", fetched_flag->item->key);
    EXPECT_EQ(2, fetched_flag->item->version);
    EXPECT_EQ(fetched_flag->version, fetched_flag->item->version);
}

TEST(DataStoreUpdaterTest, OldVersionIsDiscardedOnUpsertFlag) {
    Flag flag_a;
    flag_a.version = 2;
    flag_a.key = "flagA";
    flag_a.variations = std::vector<Value>{"potato", "ham"};

    auto store = std::make_shared<MemoryStore>();
    DataStoreUpdater updater(store, store);

    updater.Init(SDKDataSet{
        std::unordered_map<std::string, IDataStore::FlagDescriptor>{
            {"flagA", IDataStore::FlagDescriptor(flag_a)}},
        std::unordered_map<std::string, IDataStore::SegmentDescriptor>(),
    });

    Flag flag_a_2;
    flag_a_2.version = 1;
    flag_a_2.key = "flagA";
    flag_a.variations = std::vector<Value>{"potato"};

    updater.Upsert("flagA", IDataStore::FlagDescriptor(flag_a_2));

    auto fetched_flag = store->GetFlag("flagA");
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

TEST(DataStoreUpdaterTest, CanUpsertNewSegment) {
    Segment segment_a;
    segment_a.version = 1;
    segment_a.key = "segmentA";

    auto store = std::make_shared<MemoryStore>();
    DataStoreUpdater updater(store, store);

    updater.Init(SDKDataSet{
        std::unordered_map<std::string, IDataStore::FlagDescriptor>(),
        std::unordered_map<std::string, IDataStore::SegmentDescriptor>(),
    });
    updater.Upsert("segmentA", IDataStore::SegmentDescriptor(segment_a));

    auto fetched_segment = store->GetSegment("segmentA");
    EXPECT_TRUE(fetched_segment);
    EXPECT_TRUE(fetched_segment->item);
    EXPECT_EQ("segmentA", fetched_segment->item->key);
    EXPECT_EQ(1, fetched_segment->item->version);
    EXPECT_EQ(fetched_segment->version, fetched_segment->item->version);
}

TEST(DataStoreUpdaterTest, CanUpsertExitingSegment) {
    Segment segment_a;
    segment_a.version = 1;
    segment_a.key = "segmentA";

    auto store = std::make_shared<MemoryStore>();
    DataStoreUpdater updater(store, store);

    updater.Init(SDKDataSet{
        std::unordered_map<std::string, IDataStore::FlagDescriptor>(),
        std::unordered_map<std::string, IDataStore::SegmentDescriptor>{
            {"segmentA", IDataStore::SegmentDescriptor(segment_a)}},
    });

    Segment segment_a_2;
    segment_a_2.version = 2;
    segment_a_2.key = "segmentA";

    updater.Upsert("segmentA", IDataStore::SegmentDescriptor(segment_a_2));

    auto fetched_segment = store->GetSegment("segmentA");
    EXPECT_TRUE(fetched_segment);
    EXPECT_TRUE(fetched_segment->item);
    EXPECT_EQ("segmentA", fetched_segment->item->key);
    EXPECT_EQ(2, fetched_segment->item->version);
    EXPECT_EQ(fetched_segment->version, fetched_segment->item->version);
}

TEST(DataStoreUpdaterTest, OldVersionIsDiscardedOnUpsertSegment) {
    Segment segment_a;
    segment_a.version = 2;
    segment_a.key = "segmentA";

    auto store = std::make_shared<MemoryStore>();
    DataStoreUpdater updater(store, store);

    updater.Init(SDKDataSet{
        std::unordered_map<std::string, IDataStore::FlagDescriptor>(),
        std::unordered_map<std::string, IDataStore::SegmentDescriptor>{
            {"segmentA", IDataStore::SegmentDescriptor(segment_a)}},
    });

    Segment segment_a_2;
    segment_a_2.version = 1;
    segment_a_2.key = "segmentA";

    updater.Upsert("segmentA", IDataStore::SegmentDescriptor(segment_a_2));

    auto fetched_segment = store->GetSegment("segmentA");
    EXPECT_TRUE(fetched_segment);
    EXPECT_TRUE(fetched_segment->item);
    EXPECT_EQ("segmentA", fetched_segment->item->key);
    EXPECT_EQ(2, fetched_segment->item->version);
    EXPECT_EQ(fetched_segment->version, fetched_segment->item->version);
}

TEST(DataStoreUpdaterTest, ProducesChangeEventsOnUpsert) {
    Flag flag_a;
    Flag flag_b;

    flag_a.key = "flagA";
    flag_a.version = 1;

    flag_b.key = "flagB";
    flag_b.version = 1;

    flag_b.prerequisites.push_back(Flag::Prerequisite{"flagA", 0});

    auto store = std::make_shared<MemoryStore>();
    DataStoreUpdater updater(store, store);

    updater.Init(SDKDataSet{
        std::unordered_map<std::string, IDataStore::FlagDescriptor>{
            {"flagA", IDataStore::FlagDescriptor(flag_a)},
            {"flagB", IDataStore::FlagDescriptor(flag_b)}},
        std::unordered_map<std::string, IDataStore::SegmentDescriptor>(),
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

    updater.Upsert("flagA", IDataStore::FlagDescriptor(flag_a_2));

    EXPECT_EQ(true, got_event);
}

TEST(DataStoreUpdaterTest, ProducesNoEventIfNoFlagChanged) {
    Flag flag_a;
    Flag flag_b;

    flag_a.key = "flagA";
    flag_a.version = 1;

    flag_b.key = "flagB";
    flag_b.version = 1;

    flag_b.prerequisites.push_back(Flag::Prerequisite{"flagA", 0});

    auto store = std::make_shared<MemoryStore>();
    DataStoreUpdater updater(store, store);

    Segment segment_a;
    segment_a.version = 1;
    segment_a.key = "segmentA";

    updater.Init(SDKDataSet{
        std::unordered_map<std::string, IDataStore::FlagDescriptor>{
            {"flagA", IDataStore::FlagDescriptor(flag_a)},
            {"flagB", IDataStore::FlagDescriptor(flag_b)}},
        std::unordered_map<std::string, IDataStore::SegmentDescriptor>{
            {"segmentA", IDataStore::SegmentDescriptor(segment_a)},
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

    updater.Upsert("segmentA", IDataStore::SegmentDescriptor(segment_a_2));

    EXPECT_EQ(false, got_event);
}

TEST(DataStoreUpdaterTest, NoEventOnDiscardedUpsert) {
    Flag flag_a;
    Flag flag_b;

    flag_a.key = "flagA";
    flag_a.version = 1;

    flag_b.key = "flagB";
    flag_b.version = 1;

    flag_b.prerequisites.push_back(Flag::Prerequisite{"flagA", 0});

    auto store = std::make_shared<MemoryStore>();
    DataStoreUpdater updater(store, store);

    updater.Init(SDKDataSet{
        std::unordered_map<std::string, IDataStore::FlagDescriptor>{
            {"flagA", IDataStore::FlagDescriptor(flag_a)},
            {"flagB", IDataStore::FlagDescriptor(flag_b)}},
        std::unordered_map<std::string, IDataStore::SegmentDescriptor>(),
    });

    Flag flag_a_2;
    flag_a_2.key = "flagA";
    flag_a_2.version = 1;

    std::atomic<bool> got_event(false);
    updater.OnFlagChange(
        [&got_event](std::shared_ptr<std::set<std::string>> changeset) {
            got_event = true;
        });

    updater.Upsert("flagA", IDataStore::FlagDescriptor(flag_a_2));

    EXPECT_EQ(false, got_event);
}