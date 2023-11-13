#include <gtest/gtest.h>

#include <launchdarkly/logging/null_logger.hpp>

#include <data_components/memory_store/memory_store.hpp>
#include <data_systems/background_sync/sources/streaming/event_handler.hpp>

using namespace launchdarkly;
using namespace launchdarkly::server_side;
using namespace server_side::data_components;
using namespace server_side::data_systems;

TEST(DataSourceEventHandlerTests, HandlesEmptyPutMessage) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<data_components::MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    auto res = event_handler.HandleMessage("put", R"({"path":"/", "data":{}})");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kMessageHandled, res);
    ASSERT_TRUE(store->Initialized());
    EXPECT_EQ(0, store->AllFlags().size());
    EXPECT_EQ(0, store->AllSegments().size());
    EXPECT_EQ(DataSourceStatus::DataSourceState::kValid,
              manager.Status().State());
}

TEST(DataSourceEventHandlerTests, HandlesInvalidPut) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    auto res = event_handler.HandleMessage("put", "{sorry");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
    ASSERT_FALSE(store->Initialized());
    EXPECT_EQ(0, store->AllFlags().size());
    EXPECT_EQ(0, store->AllSegments().size());
    EXPECT_EQ(DataSourceStatus::DataSourceState::kInitializing,
              manager.Status().State());
}

TEST(DataSourceEventHandlerTests, HandlesInvalidPatch) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    auto res = event_handler.HandleMessage("put", "{sorry");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
    ASSERT_FALSE(store->Initialized());
    EXPECT_EQ(0, store->AllFlags().size());
    EXPECT_EQ(0, store->AllSegments().size());
    EXPECT_EQ(DataSourceStatus::DataSourceState::kInitializing,
              manager.Status().State());
}

TEST(DataSourceEventHandlerTests, HandlesPatchForUnknownPath) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    auto res = event_handler.HandleMessage(
        "patch", R"({"path":"potato", "data": "SPUD"})");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kMessageHandled, res);
    EXPECT_EQ(DataSourceStatus::DataSourceState::kInitializing,
              manager.Status().State());
}

TEST(DataSourceEventHandlerTests, HandlesPutForUnknownPath) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    auto res = event_handler.HandleMessage(
        "put", R"({"path":"potato", "data": "SPUD"})");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kMessageHandled, res);
    EXPECT_EQ(DataSourceStatus::DataSourceState::kInitializing,
              manager.Status().State());
}

TEST(DataSourceEventHandlerTests, HandlesInvalidDelete) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    auto res = event_handler.HandleMessage("put", "{sorry");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
    ASSERT_FALSE(store->Initialized());
    EXPECT_EQ(0, store->AllFlags().size());
    EXPECT_EQ(0, store->AllSegments().size());
    EXPECT_EQ(DataSourceStatus::DataSourceState::kInitializing,
              manager.Status().State());
}

TEST(DataSourceEventHandlerTests, HandlesPayloadWithFlagAndSegment) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);
    auto payload =
        R"({"path":"/","data":{"segments":{"special":{"key":"special","included":["bob"],
        "version":2}},"flags":{"HasBob":{"key":"HasBob","on":true,"fallthrough":
        {"variation":1},"variations":[true,false],"version":4}}}})";
    auto res = event_handler.HandleMessage("put", payload);

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kMessageHandled, res);
    ASSERT_TRUE(store->Initialized());
    EXPECT_EQ(1, store->AllFlags().size());
    EXPECT_EQ(1, store->AllSegments().size());
    EXPECT_TRUE(store->GetFlag("HasBob"));
    EXPECT_TRUE(store->GetSegment("special"));
    EXPECT_EQ(DataSourceStatus::DataSourceState::kValid,
              manager.Status().State());
}

TEST(DataSourceEventHandlerTests, HandlesValidFlagPatch) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    event_handler.HandleMessage("put", "{}");

    auto patch_res = event_handler.HandleMessage(
        "patch",
        R"({"path": "/flags/flagA", "data":{"key": "flagA", "version":2}})");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kMessageHandled,
              patch_res);

    EXPECT_EQ(1, store->AllFlags().size());
}

TEST(DataSourceEventHandlerTests, HandlesValidSegmentPatch) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    event_handler.HandleMessage("put", "{}");

    auto patch_res = event_handler.HandleMessage(
        "patch",
        R"({"path": "/segments/segmentA", "data":{"key": "segmentA", "version":2}})");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kMessageHandled,
              patch_res);

    EXPECT_EQ(1, store->AllSegments().size());
}

TEST(DataSourceEventHandlerTests, HandlesDeleteFlag) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    event_handler.HandleMessage(
        "put", R"({"path":"/","data":{"segments":{})"
               R"(, "flags":{"flagA": {"key":"flagA", "version": 0}}}})");

    ASSERT_TRUE(store->GetFlag("flagA")->item);

    auto patch_res = event_handler.HandleMessage(
        "delete", R"({"path": "/flags/flagA", "version": 1})");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kMessageHandled,
              patch_res);

    ASSERT_FALSE(store->GetFlag("flagA")->item);
}

TEST(DataSourceEventHandlerTests, HandlesDeleteSegment) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    event_handler.HandleMessage(
        "put",
        R"({"path":"/","data":{"flags":{})"
        R"(, "segments":{"segmentA": {"key":"segmentA", "version": 0}}}})");

    ASSERT_TRUE(store->GetSegment("segmentA")->item);

    auto patch_res = event_handler.HandleMessage(
        "delete", R"({"path": "/segments/segmentA", "version": 1})");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kMessageHandled,
              patch_res);

    ASSERT_FALSE(store->GetSegment("segmentA")->item);
}
