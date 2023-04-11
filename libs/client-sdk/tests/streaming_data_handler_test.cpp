#include <gtest/gtest.h>

#include "console_backend.hpp"
#include "context_builder.hpp"
#include "launchdarkly/client_side/data_sources/detail/streaming_data_handler.hpp"

#include <memory>

using namespace launchdarkly;
using namespace launchdarkly::client_side;
using namespace launchdarkly::client_side::data_sources::detail;

class TestHandler : public IDataSourceUpdateSink {
   public:
    void init(std::map<std::string, ItemDescriptor> data) override {
        init_data_.push_back(data);
        count_ += 1;
    }
    void upsert(std::string key, ItemDescriptor data) override {
        upsert_data_.emplace_back(key, data);
        count_ += 1;
    }

    uint64_t count_ = 0;
    std::vector<std::map<std::string, ItemDescriptor>> init_data_;
    std::vector<std::pair<std::string, ItemDescriptor>> upsert_data_;
};

TEST(StreamingDataHandlerTests, HandlesPutMessage) {
    auto logger = Logger(std::make_unique<ConsoleBackend>("test"));
    auto test_handler = std::make_shared<TestHandler>();
    StreamingDataHandler stream_handler(test_handler, logger);

    auto res = stream_handler.handle_message(launchdarkly::sse::Event(
        "put", R"({"flagA": {"version":1, "value":"test"}})"));

    EXPECT_EQ(StreamingDataHandler::MessageStatus::kMessageHandled, res);
    EXPECT_EQ(1, test_handler->count_);
    auto expected_put = std::map<std::string, ItemDescriptor>{
        {"flagA", ItemDescriptor(EvaluationResult(
                      1, std::nullopt, false, false, std::nullopt,
                      EvaluationDetailInternal(Value("test"), std::nullopt,
                                               std::nullopt)))}};
    EXPECT_EQ(expected_put, test_handler->init_data_[0]);
}

TEST(StreamingDataHandlerTests, HandlesEmptyPutMessage) {
    auto logger = Logger(std::make_unique<ConsoleBackend>("test"));
    auto test_handler = std::make_shared<TestHandler>();
    StreamingDataHandler stream_handler(test_handler, logger);

    auto res =
        stream_handler.handle_message(launchdarkly::sse::Event("put", "{}"));

    EXPECT_EQ(StreamingDataHandler::MessageStatus::kMessageHandled, res);
    EXPECT_EQ(1, test_handler->count_);
    auto expected_put = std::map<std::string, ItemDescriptor>();
    EXPECT_EQ(expected_put, test_handler->init_data_[0]);
}

TEST(StreamingDataHandlerTests, BadJsonPut) {
    auto logger = Logger(std::make_unique<ConsoleBackend>("test"));
    auto test_handler = std::make_shared<TestHandler>();
    StreamingDataHandler stream_handler(test_handler, logger);

    auto res = stream_handler.handle_message(
        launchdarkly::sse::Event("put", "{sorry"));

    EXPECT_EQ(StreamingDataHandler::MessageStatus::kInvalidMessage, res);
    EXPECT_EQ(0, test_handler->count_);
}

TEST(StreamingDataHandlerTests, BadSchemaPut) {
    auto logger = Logger(std::make_unique<ConsoleBackend>("test"));
    auto test_handler = std::make_shared<TestHandler>();
    StreamingDataHandler stream_handler(test_handler, logger);

    auto res = stream_handler.handle_message(
        launchdarkly::sse::Event("put", "{\"potato\": {}}"));

    EXPECT_EQ(StreamingDataHandler::MessageStatus::kInvalidMessage, res);
    EXPECT_EQ(0, test_handler->count_);
}

TEST(StreamingDataHandlerTests, HandlesPatchMessage) {
    auto logger = Logger(std::make_unique<ConsoleBackend>("test"));
    auto test_handler = std::make_shared<TestHandler>();
    StreamingDataHandler stream_handler(test_handler, logger);

    auto res = stream_handler.handle_message(launchdarkly::sse::Event(
        "patch", R"({"key": "flagA", "version":1, "value": "test"})"));

    EXPECT_EQ(StreamingDataHandler::MessageStatus::kMessageHandled, res);
    EXPECT_EQ(1, test_handler->count_);
    auto expected_put = std::pair<std::string, ItemDescriptor>{
        "flagA", ItemDescriptor(EvaluationResult(
                     1, std::nullopt, false, false, std::nullopt,
                     EvaluationDetailInternal(Value("test"), std::nullopt,
                                              std::nullopt)))};
    EXPECT_EQ(expected_put, test_handler->upsert_data_[0]);
}

TEST(StreamingDataHandlerTests, BadJsonPatch) {
    auto logger = Logger(std::make_unique<ConsoleBackend>("test"));
    auto test_handler = std::make_shared<TestHandler>();
    StreamingDataHandler stream_handler(test_handler, logger);

    auto res = stream_handler.handle_message(
        launchdarkly::sse::Event("patch", "{sorry"));

    EXPECT_EQ(StreamingDataHandler::MessageStatus::kInvalidMessage, res);
    EXPECT_EQ(0, test_handler->count_);
}

TEST(StreamingDataHandlerTests, BadSchemaPatch) {
    auto logger = Logger(std::make_unique<ConsoleBackend>("test"));
    auto test_handler = std::make_shared<TestHandler>();
    StreamingDataHandler stream_handler(test_handler, logger);

    auto res = stream_handler.handle_message(
        launchdarkly::sse::Event("patch", R"({"potato": {}})"));

    EXPECT_EQ(StreamingDataHandler::MessageStatus::kInvalidMessage, res);
    EXPECT_EQ(0, test_handler->count_);
}

TEST(StreamingDataHandlerTests, HandlesDeleteMessage) {
    auto logger = Logger(std::make_unique<ConsoleBackend>("test"));
    auto test_handler = std::make_shared<TestHandler>();
    StreamingDataHandler stream_handler(test_handler, logger);

    auto res = stream_handler.handle_message(
        launchdarkly::sse::Event("delete", R"({"key": "flagA", "version":1})"));

    EXPECT_EQ(StreamingDataHandler::MessageStatus::kMessageHandled, res);
    EXPECT_EQ(1, test_handler->count_);
    auto expected_put =
        std::pair<std::string, ItemDescriptor>{"flagA", ItemDescriptor(1)};
    EXPECT_EQ(expected_put, test_handler->upsert_data_[0]);
}

TEST(StreamingDataHandlerTests, BadJsonDelete) {
    auto logger = Logger(std::make_unique<ConsoleBackend>("test"));
    auto test_handler = std::make_shared<TestHandler>();
    StreamingDataHandler stream_handler(test_handler, logger);

    auto res = stream_handler.handle_message(
        launchdarkly::sse::Event("delete", "{sorry"));

    EXPECT_EQ(StreamingDataHandler::MessageStatus::kInvalidMessage, res);
    EXPECT_EQ(0, test_handler->count_);
}

TEST(StreamingDataHandlerTests, BadSchemaDelete) {
    auto logger = Logger(std::make_unique<ConsoleBackend>("test"));
    auto test_handler = std::make_shared<TestHandler>();
    StreamingDataHandler stream_handler(test_handler, logger);

    auto res = stream_handler.handle_message(
        launchdarkly::sse::Event("delete", R"({"potato": {}})"));

    EXPECT_EQ(StreamingDataHandler::MessageStatus::kInvalidMessage, res);
    EXPECT_EQ(0, test_handler->count_);
}

TEST(StreamingDataHandlerTests, UnrecognizedVerb) {
    auto logger = Logger(std::make_unique<ConsoleBackend>("test"));
    auto test_handler = std::make_shared<TestHandler>();
    StreamingDataHandler stream_handler(test_handler, logger);

    auto res = stream_handler.handle_message(
        launchdarkly::sse::Event("potato", R"({"potato": {}})"));

    EXPECT_EQ(StreamingDataHandler::MessageStatus::kUnhandledVerb, res);
    EXPECT_EQ(0, test_handler->count_);
}
