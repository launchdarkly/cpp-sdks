#include <gtest/gtest.h>

#include "data_sources/data_source_event_handler.hpp"

#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/logging/console_backend.hpp>

#include <memory>
#include <unordered_map>

using namespace launchdarkly;
using namespace launchdarkly::client_side;
using namespace launchdarkly::client_side::data_sources;

class TestHandler : public IDataSourceUpdateSink {
   public:
    void Init(
        Context const& context,
        std::unordered_map<std::string, FlagItemDescriptor> data) override {
        init_data_.push_back(data);
        count_ += 1;
    }
    void Upsert(Context const& context,
                std::string key,
                FlagItemDescriptor data) override {
        upsert_data_.emplace_back(key, data);
        count_ += 1;
    }

    uint64_t count_ = 0;
    std::vector<std::unordered_map<std::string, FlagItemDescriptor>> init_data_;
    std::vector<std::pair<std::string, FlagItemDescriptor>> upsert_data_;
};

TEST(StreamingDataHandlerTests, HandlesPutMessage) {
    auto logger = Logger(std::make_shared<logging::ConsoleBackend>("test"));
    auto test_handler = TestHandler();
    DataSourceStatusManager status_manager;
    DataSourceEventHandler stream_handler(
        ContextBuilder().Kind("user", "user-key").Build(), test_handler, logger,
        status_manager);

    auto res = stream_handler.HandleMessage(
        "put", R"({"flagA": {"version":1, "value":"test"}})");

    EXPECT_EQ(DataSourceEventHandler::MessageStatus::kMessageHandled, res);
    EXPECT_EQ(1, test_handler.count_);
    auto expected_put = std::unordered_map<std::string, FlagItemDescriptor>{
        {"flagA", FlagItemDescriptor(EvaluationResult(
                      1, std::nullopt, false, false, std::nullopt,
                      EvaluationDetailInternal(Value("test"), std::nullopt,
                                               std::nullopt)))}};
    EXPECT_EQ(expected_put, test_handler.init_data_[0]);
}

TEST(StreamingDataHandlerTests, HandlesEmptyPutMessage) {
    auto logger = Logger(std::make_shared<logging::ConsoleBackend>("test"));
    auto test_handler = TestHandler();
    DataSourceStatusManager status_manager;
    DataSourceEventHandler stream_handler(
        ContextBuilder().Kind("user", "user-key").Build(), test_handler, logger,
        status_manager);

    auto res = stream_handler.HandleMessage("put", "{}");

    EXPECT_EQ(DataSourceEventHandler::MessageStatus::kMessageHandled, res);
    EXPECT_EQ(1, test_handler.count_);
    auto expected_put = std::unordered_map<std::string, FlagItemDescriptor>();
    EXPECT_EQ(expected_put, test_handler.init_data_[0]);
}

TEST(StreamingDataHandlerTests, BadJsonPut) {
    auto logger = Logger(std::make_shared<logging::ConsoleBackend>("test"));
    auto test_handler = TestHandler();
    DataSourceStatusManager status_manager;
    DataSourceEventHandler stream_handler(
        ContextBuilder().Kind("user", "user-key").Build(), test_handler, logger,
        status_manager);

    auto res = stream_handler.HandleMessage("put", "{sorry");

    EXPECT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
    EXPECT_EQ(0, test_handler.count_);
}

TEST(StreamingDataHandlerTests, BadSchemaPut) {
    auto logger = Logger(std::make_shared<logging::ConsoleBackend>("test"));
    auto test_handler = TestHandler();
    DataSourceStatusManager status_manager;
    DataSourceEventHandler stream_handler(
        ContextBuilder().Kind("user", "user-key").Build(), test_handler, logger,
        status_manager);

    auto res = stream_handler.HandleMessage("put", "{\"potato\": {}}");

    EXPECT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
    EXPECT_EQ(0, test_handler.count_);
}

TEST(StreamingDataHandlerTests, HandlesPatchMessage) {
    auto logger = Logger(std::make_shared<logging::ConsoleBackend>("test"));
    auto test_handler = TestHandler();
    DataSourceStatusManager status_manager;
    DataSourceEventHandler stream_handler(
        ContextBuilder().Kind("user", "user-key").Build(), test_handler, logger,
        status_manager);

    auto res = stream_handler.HandleMessage(
        "patch", R"({"key": "flagA", "version":1, "value": "test"})");

    EXPECT_EQ(DataSourceEventHandler::MessageStatus::kMessageHandled, res);
    EXPECT_EQ(1, test_handler.count_);
    auto expected_put = std::pair<std::string, FlagItemDescriptor>{
        "flagA", FlagItemDescriptor(EvaluationResult(
                     1, std::nullopt, false, false, std::nullopt,
                     EvaluationDetailInternal(Value("test"), std::nullopt,
                                              std::nullopt)))};
    EXPECT_EQ(expected_put, test_handler.upsert_data_[0]);
}

TEST(StreamingDataHandlerTests, BadJsonPatch) {
    auto logger = Logger(std::make_shared<logging::ConsoleBackend>("test"));
    auto test_handler = TestHandler();
    DataSourceStatusManager status_manager;
    DataSourceEventHandler stream_handler(
        ContextBuilder().Kind("user", "user-key").Build(), test_handler, logger,
        status_manager);

    auto res = stream_handler.HandleMessage("patch", "{sorry");

    EXPECT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
    EXPECT_EQ(0, test_handler.count_);
}

TEST(StreamingDataHandlerTests, BadSchemaPatch) {
    auto logger = Logger(std::make_shared<logging::ConsoleBackend>("test"));
    auto test_handler = TestHandler();
    DataSourceStatusManager status_manager;
    DataSourceEventHandler stream_handler(
        ContextBuilder().Kind("user", "user-key").Build(), test_handler, logger,
        status_manager);

    auto res = stream_handler.HandleMessage("patch", R"({"potato": {}})");

    EXPECT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
    EXPECT_EQ(0, test_handler.count_);
}

TEST(StreamingDataHandlerTests, HandlesDeleteMessage) {
    auto logger = Logger(std::make_shared<logging::ConsoleBackend>("test"));
    auto test_handler = TestHandler();
    DataSourceStatusManager status_manager;
    DataSourceEventHandler stream_handler(
        ContextBuilder().Kind("user", "user-key").Build(), test_handler, logger,
        status_manager);

    auto res = stream_handler.HandleMessage("delete",
                                            R"({"key": "flagA", "version":1})");

    EXPECT_EQ(DataSourceEventHandler::MessageStatus::kMessageHandled, res);
    EXPECT_EQ(1, test_handler.count_);
    auto expected_put = std::pair<std::string, FlagItemDescriptor>{
        "flagA", FlagItemDescriptor(1)};
    EXPECT_EQ(expected_put, test_handler.upsert_data_[0]);
}

TEST(StreamingDataHandlerTests, BadJsonDelete) {
    auto logger = Logger(std::make_shared<logging::ConsoleBackend>("test"));
    auto test_handler = TestHandler();
    DataSourceStatusManager status_manager;
    DataSourceEventHandler stream_handler(
        ContextBuilder().Kind("user", "user-key").Build(), test_handler, logger,
        status_manager);

    auto res = stream_handler.HandleMessage("delete", "{sorry");

    EXPECT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
    EXPECT_EQ(0, test_handler.count_);
}

TEST(StreamingDataHandlerTests, BadSchemaDelete) {
    auto logger = Logger(std::make_shared<logging::ConsoleBackend>("test"));
    auto test_handler = TestHandler();
    DataSourceStatusManager status_manager;
    DataSourceEventHandler stream_handler(
        ContextBuilder().Kind("user", "user-key").Build(), test_handler, logger,
        status_manager);

    auto res = stream_handler.HandleMessage("delete", R"({"potato": {}})");

    EXPECT_EQ(DataSourceEventHandler::MessageStatus::kInvalidMessage, res);
    EXPECT_EQ(0, test_handler.count_);
}

TEST(StreamingDataHandlerTests, UnrecognizedVerb) {
    auto logger = Logger(std::make_shared<logging::ConsoleBackend>("test"));
    auto test_handler = TestHandler();
    DataSourceStatusManager status_manager;
    DataSourceEventHandler stream_handler(
        ContextBuilder().Kind("user", "user-key").Build(), test_handler, logger,
        status_manager);

    auto res = stream_handler.HandleMessage("potato", R"({"potato": {}})");

    EXPECT_EQ(DataSourceEventHandler::MessageStatus::kUnhandledVerb, res);
    EXPECT_EQ(0, test_handler.count_);
}
