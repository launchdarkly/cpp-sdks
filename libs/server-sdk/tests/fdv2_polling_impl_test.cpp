#include <gtest/gtest.h>

#include <data_systems/fdv2/fdv2_polling_impl.hpp>

#include <launchdarkly/config/shared/defaults.hpp>
#include <launchdarkly/fdv2_protocol_handler.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/network/http_requester.hpp>

using namespace launchdarkly;
using namespace launchdarkly::server_side::data_interfaces;
using namespace launchdarkly::server_side::data_systems;

static Logger MakeNullLogger() {
    struct NullBackend : ILogBackend {
        bool Enabled(LogLevel) noexcept override { return false; }
        void Write(LogLevel, std::string) noexcept override {}
    };
    return Logger{std::make_shared<NullBackend>()};
}

static FDv2SourceResult HandleResponse(
    unsigned status,
    std::optional<std::string> body,
    network::HttpResult::HeadersType headers) {
    auto logger = MakeNullLogger();
    FDv2ProtocolHandler handler;
    network::HttpResult res{status, std::move(body), std::move(headers)};
    return HandleFDv2PollResponse(res, &handler, logger, "test");
}

TEST(HandleFDv2PollResponseTest, NotModifiedSetsFdv1FallbackWhenHeaderPresent) {
    auto result =
        HandleResponse(304, std::nullopt, {{"X-LD-FD-Fallback", "true"}});
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::ChangeSet>(result.value));
    EXPECT_TRUE(result.fdv1_fallback);
}

TEST(HandleFDv2PollResponseTest, FlagIsFalseWhenHeaderAbsent) {
    auto result = HandleResponse(304, std::nullopt, {});
    EXPECT_FALSE(result.fdv1_fallback);
}

TEST(HandleFDv2PollResponseTest, RecoverableErrorPropagatesFlag) {
    auto result =
        HandleResponse(400, std::nullopt, {{"x-ld-fd-fallback", "true"}});
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Interrupted>(result.value));
    EXPECT_TRUE(result.fdv1_fallback);
}

TEST(HandleFDv2PollResponseTest, TerminalErrorPropagatesFlag) {
    auto result =
        HandleResponse(401, std::nullopt, {{"X-LD-FD-Fallback", "true"}});
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::TerminalError>(result.value));
    EXPECT_TRUE(result.fdv1_fallback);
}

TEST(HandleFDv2PollResponseTest, OkWithChangeSetBodyPropagatesFlag) {
    std::string const body =
        R"({"events":[)"
        R"({"event":"server-intent","data":{"payloads":[)"
        R"({"id":"p1","target":1,"intentCode":"xfer-full"}]}},)"
        R"({"event":"put-object","data":{"version":1,"kind":"flag",)"
        R"("key":"my-flag","object":{"key":"my-flag","on":true,)"
        R"("fallthrough":{"variation":0},"variations":[true,false],)"
        R"("version":1}}},)"
        R"({"event":"payload-transferred","data":{"state":"abc","version":1}})"
        R"(]})";

    auto result = HandleResponse(200, body, {{"X-LD-FD-Fallback", "true"}});

    auto* change_set = std::get_if<FDv2SourceResult::ChangeSet>(&result.value);
    ASSERT_NE(change_set, nullptr);
    EXPECT_FALSE(change_set->change_set.data.empty());
    EXPECT_TRUE(result.fdv1_fallback);
}

TEST(HandleFDv2PollResponseTest, OkWithMissingBodyPropagatesFlag) {
    auto result =
        HandleResponse(200, std::nullopt, {{"X-LD-FD-Fallback", "true"}});
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Interrupted>(result.value));
    EXPECT_TRUE(result.fdv1_fallback);
}

TEST(HandleFDv2PollResponseTest, HeaderValueIsCaseInsensitive) {
    auto result =
        HandleResponse(304, std::nullopt, {{"X-LD-FD-Fallback", "TRUE"}});
    EXPECT_TRUE(result.fdv1_fallback);
}

TEST(HandleFDv2PollResponseTest, HeaderValueOtherThanTrueDoesNotSetFlag) {
    auto result =
        HandleResponse(304, std::nullopt, {{"X-LD-FD-Fallback", "false"}});
    EXPECT_FALSE(result.fdv1_fallback);
}

TEST(HandleFDv2PollResponseTest, NetworkErrorDoesNotSetFlag) {
    auto logger = MakeNullLogger();
    FDv2ProtocolHandler handler;
    network::HttpResult res{std::optional<std::string>{"connection refused"}};
    auto result = HandleFDv2PollResponse(res, &handler, logger, "test");

    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Interrupted>(result.value));
    EXPECT_FALSE(result.fdv1_fallback);
}

TEST(MakeFDv2PollRequestTest, BaseWithTrailingSlashDoesNotProduceDoubleSlash) {
    auto logger = MakeNullLogger();
    config::shared::built::ServiceEndpoints endpoints{"http://example.com/", "",
                                                      ""};
    auto props =
        config::shared::Defaults<config::shared::ServerSDK>::HttpProperties();
    auto req = MakeFDv2PollRequest(endpoints, props, data_model::Selector{},
                                   std::nullopt, logger);
    EXPECT_EQ(req.Url(), "http://example.com/sdk/poll");
}

TEST(MakeFDv2PollRequestTest, BaseWithSubpathTrailingSlashJoinsCleanly) {
    auto logger = MakeNullLogger();
    config::shared::built::ServiceEndpoints endpoints{
        "http://example.com/relay/", "", ""};
    auto props =
        config::shared::Defaults<config::shared::ServerSDK>::HttpProperties();
    auto req = MakeFDv2PollRequest(endpoints, props, data_model::Selector{},
                                   std::nullopt, logger);
    EXPECT_EQ(req.Url(), "http://example.com/relay/sdk/poll");
}

TEST(MakeFDv2PollRequestTest, ValidFilterKeyIsIncluded) {
    auto logger = MakeNullLogger();
    config::shared::built::ServiceEndpoints endpoints{"http://example.com", "",
                                                      ""};
    auto props =
        config::shared::Defaults<config::shared::ServerSDK>::HttpProperties();
    auto req = MakeFDv2PollRequest(endpoints, props, data_model::Selector{},
                                   std::string{"my-filter_1.0"}, logger);
    EXPECT_EQ(req.Url(), "http://example.com/sdk/poll?filter=my-filter_1.0");
}

TEST(MakeFDv2PollRequestTest, InvalidFilterKeyIsDropped) {
    auto logger = MakeNullLogger();
    config::shared::built::ServiceEndpoints endpoints{"http://example.com", "",
                                                      ""};
    auto props =
        config::shared::Defaults<config::shared::ServerSDK>::HttpProperties();
    auto req = MakeFDv2PollRequest(endpoints, props, data_model::Selector{},
                                   std::string{"has spaces"}, logger);
    EXPECT_EQ(req.Url(), "http://example.com/sdk/poll");
}
