#include <gtest/gtest.h>

#include "data_store/memory_store.hpp"

using launchdarkly::data_model::SDKDataSet;
using launchdarkly::server_side::data_store::IDataStore;
using launchdarkly::server_side::data_store::MemoryStore;

using launchdarkly::Value;
using launchdarkly::data_model::Flag;
using launchdarkly::data_model::Segment;

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
    EXPECT_EQ(std::string("memory"), store.Description());
}

TEST(MemoryStoreTest, CanGetFlag) {
    MemoryStore store;
    Flag flag;
    flag.version = 1;
    flag.key = "flagA";
    flag.on = true;
    flag.variations = std::vector<Value>{true, false};
    flag.fallthrough = 0;
    store.Init(SDKDataSet{
        std::unordered_map<std::string, IDataStore::FlagDescriptor>{
            {"flagA", IDataStore::FlagDescriptor(flag)}},
        std::unordered_map<std::string, IDataStore::SegmentDescriptor>(),
    });

    auto fetchedFlag = store.GetFlag("flagA");
    EXPECT_TRUE(fetchedFlag);
    EXPECT_TRUE(fetchedFlag->item);
    EXPECT_EQ("flagA", fetchedFlag->item->key);
    EXPECT_EQ(1, fetchedFlag->item->version);
    EXPECT_EQ(fetchedFlag->version, fetchedFlag->item->version);
}

TEST(MemoryStoreTest, CanGetAllFlags) {
    Flag flagA;
    flagA.version = 1;
    flagA.key = "flagA";

    Flag flagB;
    flagB.version = 2;
    flagB.key = "flagB";

    MemoryStore store;
    store.Init(SDKDataSet{
        std::unordered_map<std::string, IDataStore::FlagDescriptor>{
            {"flagA", IDataStore::FlagDescriptor(flagA)},
            {"flagB", IDataStore::FlagDescriptor(flagB)}},
        std::unordered_map<std::string, IDataStore::SegmentDescriptor>(),
    });

    auto fetched = store.AllFlags();
    EXPECT_EQ(2, fetched.size());

    EXPECT_EQ(std::string("flagA"), fetched["flagA"]->item->key);
    EXPECT_EQ(std::string("flagB"), fetched["flagB"]->item->key);
}

TEST(MemoryStoreTest, CanGetAllFlagsWhenThereAreNoFlags) {
    MemoryStore store;
    store.Init(SDKDataSet{
        std::unordered_map<std::string, IDataStore::FlagDescriptor>(),
        std::unordered_map<std::string, IDataStore::SegmentDescriptor>(),
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
        std::unordered_map<std::string, IDataStore::FlagDescriptor>(),
        std::unordered_map<std::string, IDataStore::SegmentDescriptor>{
            {"segmentA", IDataStore::SegmentDescriptor(segment)}},
    });

    auto fetchedSegment = store.GetSegment("segmentA");
    EXPECT_TRUE(fetchedSegment);
    EXPECT_TRUE(fetchedSegment->item);
    EXPECT_EQ("segmentA", fetchedSegment->item->key);
    EXPECT_EQ(1, fetchedSegment->item->version);
    EXPECT_EQ(fetchedSegment->version, fetchedSegment->item->version);
}

TEST(MemoryStoreTest, CanGetAllSegments) {
    auto segmentA = Segment();
    segmentA.version = 1;
    segmentA.key = "segmentA";

    auto segmentB = Segment();
    segmentB.version = 2;
    segmentB.key = "segmentB";

    MemoryStore store;
    store.Init(SDKDataSet{
        std::unordered_map<std::string, IDataStore::FlagDescriptor>(),
        std::unordered_map<std::string, IDataStore::SegmentDescriptor>{
            {"segmentA", IDataStore::SegmentDescriptor(segmentA)},
            {"segmentB", IDataStore::SegmentDescriptor(segmentB)}},
    });

    auto fetched = store.AllSegments();
    EXPECT_EQ(2, fetched.size());

    EXPECT_EQ(std::string("segmentA"), fetched["segmentA"]->item->key);
    EXPECT_EQ(std::string("segmentB"), fetched["segmentB"]->item->key);
}

TEST(MemoryStoreTest, CanGetAllSegmentsWhenThereAreNoSegments) {
    MemoryStore store;
    store.Init(SDKDataSet{
        std::unordered_map<std::string, IDataStore::FlagDescriptor>(),
        std::unordered_map<std::string, IDataStore::SegmentDescriptor>(),
    });

    auto fetched = store.AllSegments();
    EXPECT_EQ(0, fetched.size());
}

TEST(MemoryStoreTest, GetMissingFlagOrSegment) {
    MemoryStore store;
    auto fetchedFlag = store.GetFlag("flagA");
    EXPECT_FALSE(fetchedFlag);
    auto fetchedSegment = store.GetSegment("segmentA");
    EXPECT_FALSE(fetchedSegment);
}

TEST(MemoryStoreTest, CanUpsertNewFlag) {
    Flag flagA;
    flagA.version = 1;
    flagA.key = "flagA";

    MemoryStore store;
    store.Init(SDKDataSet{
        std::unordered_map<std::string, IDataStore::FlagDescriptor>(),
        std::unordered_map<std::string, IDataStore::SegmentDescriptor>(),
    });
    store.Upsert("flagA", IDataStore::FlagDescriptor(flagA));

    auto fetchedFlag = store.GetFlag("flagA");
    EXPECT_TRUE(fetchedFlag);
    EXPECT_TRUE(fetchedFlag->item);
    EXPECT_EQ("flagA", fetchedFlag->item->key);
    EXPECT_EQ(1, fetchedFlag->item->version);
    EXPECT_EQ(fetchedFlag->version, fetchedFlag->item->version);
}

TEST(MemoryStoreTest, CanUpsertExitingFlag) {
    Flag flagA;
    flagA.version = 1;
    flagA.key = "flagA";

    MemoryStore store;
    store.Init(SDKDataSet{
        std::unordered_map<std::string, IDataStore::FlagDescriptor>{
            {"flagA", IDataStore::FlagDescriptor(flagA)}},
        std::unordered_map<std::string, IDataStore::SegmentDescriptor>(),
    });

    Flag flagA2;
    flagA2.version = 2;
    flagA2.key = "flagA";

    store.Upsert("flagA", IDataStore::FlagDescriptor(flagA2));

    auto fetchedFlag = store.GetFlag("flagA");
    EXPECT_TRUE(fetchedFlag);
    EXPECT_TRUE(fetchedFlag->item);
    EXPECT_EQ("flagA", fetchedFlag->item->key);
    EXPECT_EQ(2, fetchedFlag->item->version);
    EXPECT_EQ(fetchedFlag->version, fetchedFlag->item->version);
}

TEST(MemoryStoreTest, CanUpsertNewSegment) {
    Segment segmentA;
    segmentA.version = 1;
    segmentA.key = "segmentA";

    MemoryStore store;
    store.Init(SDKDataSet{
        std::unordered_map<std::string, IDataStore::FlagDescriptor>(),
        std::unordered_map<std::string, IDataStore::SegmentDescriptor>(),
    });
    store.Upsert("segmentA", IDataStore::SegmentDescriptor(segmentA));

    auto fetchedSegment = store.GetSegment("segmentA");
    EXPECT_TRUE(fetchedSegment);
    EXPECT_TRUE(fetchedSegment->item);
    EXPECT_EQ("segmentA", fetchedSegment->item->key);
    EXPECT_EQ(1, fetchedSegment->item->version);
    EXPECT_EQ(fetchedSegment->version, fetchedSegment->item->version);
}

TEST(MemoryStoreTest, CanUpsertExitingSegment) {
    Segment segmentA;
    segmentA.version = 1;
    segmentA.key = "segmentA";

    MemoryStore store;
    store.Init(SDKDataSet{
        std::unordered_map<std::string, IDataStore::FlagDescriptor>(),
        std::unordered_map<std::string, IDataStore::SegmentDescriptor>{
            {"segmentA", IDataStore::SegmentDescriptor(segmentA)}},
    });

    Segment segmentA2;
    segmentA2.version = 2;
    segmentA2.key = "segmentA";

    store.Upsert("segmentA", IDataStore::SegmentDescriptor(segmentA2));

    auto fetchedSegment = store.GetSegment("segmentA");
    EXPECT_TRUE(fetchedSegment);
    EXPECT_TRUE(fetchedSegment->item);
    EXPECT_EQ("segmentA", fetchedSegment->item->key);
    EXPECT_EQ(2, fetchedSegment->item->version);
    EXPECT_EQ(fetchedSegment->version, fetchedSegment->item->version);
}
