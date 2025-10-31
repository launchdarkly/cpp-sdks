#include <gtest/gtest.h>

#include "data_components/memory_store/memory_store.hpp"
#include "data_systems/background_sync/sources/streaming/event_handler.hpp"

#include <launchdarkly/logging/null_logger.hpp>

#include <memory>

using namespace launchdarkly;
using namespace launchdarkly::server_side;
using namespace server_side::data_components;
using namespace server_side::data_systems;

TEST(DataSourceEventHandlerTests, HandlesEmptyPutMessage) {
    auto logger = logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
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

TEST(DataSourceEventHandlerTests, HandlesPatchWithNullDataForFlag) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    // Initialize the store
    event_handler.HandleMessage("put", R"({"path":"/", "data":{}})");

    // Null data should be treated as invalid, not crash the application
    auto res = event_handler.HandleMessage(
        "patch", R"({"path": "/flags/flagA", "data": null})");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
    // The error should be recorded, but we stay in Valid state after a previous successful PUT
    EXPECT_EQ(DataSourceStatus::DataSourceState::kValid,
              manager.Status().State());
    ASSERT_TRUE(manager.Status().LastError().has_value());
    EXPECT_EQ(DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
              manager.Status().LastError()->Kind());
}

TEST(DataSourceEventHandlerTests, HandlesPatchWithNullDataForSegment) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    // Initialize the store
    event_handler.HandleMessage("put", R"({"path":"/", "data":{}})");

    // Null data should be treated as invalid, not crash the application
    auto res = event_handler.HandleMessage(
        "patch", R"({"path": "/segments/segmentA", "data": null})");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
    // The error should be recorded, but we stay in Valid state after a previous successful PUT
    EXPECT_EQ(DataSourceStatus::DataSourceState::kValid,
              manager.Status().State());
    ASSERT_TRUE(manager.Status().LastError().has_value());
    EXPECT_EQ(DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
              manager.Status().LastError()->Kind());
}

TEST(DataSourceEventHandlerTests, HandlesPatchWithMissingDataField) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    // Initialize the store
    event_handler.HandleMessage("put", R"({"path":"/", "data":{}})");

    // Missing data field should also be treated as invalid
    auto res = event_handler.HandleMessage(
        "patch", R"({"path": "/flags/flagA"})");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
}

TEST(DataSourceEventHandlerTests, HandlesPutWithNullData) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    // PUT with null data should also be handled safely
    auto res = event_handler.HandleMessage(
        "put", R"({"path":"/", "data": null})");

    // PUT handles this differently - it may succeed with empty data
    // The important thing is it doesn't crash
    ASSERT_TRUE(res == DataSourceEventHandler::MessageStatus::kMessageHandled ||
                res == DataSourceEventHandler::MessageStatus::kInvalidMessage);
}

// Tests for wrong data types (schema validation errors)

TEST(DataSourceEventHandlerTests, HandlesPatchWithBooleanData) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    // Initialize the store
    event_handler.HandleMessage("put", R"({"path":"/", "data":{}})");

    // Boolean data instead of object should be treated as invalid
    auto res = event_handler.HandleMessage(
        "patch", R"({"path": "/flags/flagA", "data": true})");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
}

TEST(DataSourceEventHandlerTests, HandlesPatchWithStringData) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    // Initialize the store
    event_handler.HandleMessage("put", R"({"path":"/", "data":{}})");

    // String data instead of object should be treated as invalid
    auto res = event_handler.HandleMessage(
        "patch", R"({"path": "/flags/flagA", "data": "not an object"})");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
}

TEST(DataSourceEventHandlerTests, HandlesPatchWithArrayData) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    // Initialize the store
    event_handler.HandleMessage("put", R"({"path":"/", "data":{}})");

    // Array data instead of object should be treated as invalid
    auto res = event_handler.HandleMessage(
        "patch", R"({"path": "/flags/flagA", "data": []})");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
}

TEST(DataSourceEventHandlerTests, HandlesPatchWithNumberData) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    // Initialize the store
    event_handler.HandleMessage("put", R"({"path":"/", "data":{}})");

    // Number data instead of object should be treated as invalid
    auto res = event_handler.HandleMessage(
        "patch", R"({"path": "/flags/flagA", "data": 42})");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
}

TEST(DataSourceEventHandlerTests, HandlesDeleteWithStringVersion) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    // Initialize the store
    event_handler.HandleMessage("put", R"({"path":"/", "data":{}})");

    // String version instead of number should be treated as invalid
    auto res = event_handler.HandleMessage(
        "delete", R"({"path": "/flags/flagA", "version": "not a number"})");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
}

TEST(DataSourceEventHandlerTests, HandlesPutWithInvalidFlagsType) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    // Flags should be an object, not a boolean
    auto res = event_handler.HandleMessage(
        "put", R"({"path": "/", "data": {"flags": true, "segments": {}}})");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
}

TEST(DataSourceEventHandlerTests, HandlesPutWithInvalidSegmentsType) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    // Segments should be an object, not an array
    auto res = event_handler.HandleMessage(
        "put", R"({"path": "/", "data": {"flags": {}, "segments": []}})");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
}

// Tests for additional malformed JSON variants

TEST(DataSourceEventHandlerTests, HandlesUnterminatedString) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    // Unterminated string should be treated as malformed JSON
    auto res = event_handler.HandleMessage(
        "patch", R"({"path": "/flags/x", "data": "unterminated)");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
}

TEST(DataSourceEventHandlerTests, HandlesTrailingComma) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    // Trailing comma should be treated as malformed JSON
    auto res = event_handler.HandleMessage(
        "patch", R"({"path": "/flags/x", "data": {},})");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
}

// Tests for missing required fields

TEST(DataSourceEventHandlerTests, HandlesDeleteWithMissingPath) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    // Initialize the store
    event_handler.HandleMessage("put", R"({"path":"/", "data":{}})");

    // Missing path field should be treated as invalid
    auto res = event_handler.HandleMessage(
        "delete", R"({"version": 1})");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
}

TEST(DataSourceEventHandlerTests, HandlesDeleteWithMissingVersion) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    // Initialize the store
    event_handler.HandleMessage("put", R"({"path":"/", "data":{}})");

    // Missing version field should be treated as invalid
    auto res = event_handler.HandleMessage(
        "delete", R"({"path": "/flags/flagA"})");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
}

TEST(DataSourceEventHandlerTests, HandlesPutWithMissingPath) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    // Missing/empty path is treated as unrecognized (safely ignored)
    // This provides forward compatibility
    auto res = event_handler.HandleMessage(
        "put", R"({"data": {}})");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kMessageHandled, res);
}

TEST(DataSourceEventHandlerTests, HandlesEmptyJsonObject) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;
    DataSourceEventHandler event_handler(*store, logger, manager);

    // Empty JSON object with missing path is treated as unrecognized (safely ignored)
    // This provides forward compatibility with future event types
    auto res = event_handler.HandleMessage("patch", "{}");

    ASSERT_EQ(DataSourceEventHandler::MessageStatus::kMessageHandled, res);
}
