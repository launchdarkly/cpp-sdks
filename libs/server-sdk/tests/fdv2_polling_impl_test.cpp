#include <gtest/gtest.h>

#include <data_systems/fdv2/fdv2_polling_impl.hpp>

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

TEST(HandleFDv2PollResponseTest, NotModifiedSetsFdv1FallbackWhenHeaderPresent) {
    auto logger = MakeNullLogger();
    FDv2ProtocolHandler handler;

    network::HttpResult::HeadersType headers{{"X-LD-FD-Fallback", "true"}};
    network::HttpResult res{304, std::nullopt, std::move(headers)};

    auto result = HandleFDv2PollResponse(res, &handler, logger, "test");

    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::ChangeSet>(result.value));
    EXPECT_TRUE(result.fdv1_fallback);
}

TEST(HandleFDv2PollResponseTest, FlagIsFalseWhenHeaderAbsent) {
    auto logger = MakeNullLogger();
    FDv2ProtocolHandler handler;

    network::HttpResult res{304, std::nullopt, {}};

    auto result = HandleFDv2PollResponse(res, &handler, logger, "test");

    EXPECT_FALSE(result.fdv1_fallback);
}

TEST(HandleFDv2PollResponseTest, RecoverableErrorPropagatesFlag) {
    auto logger = MakeNullLogger();
    FDv2ProtocolHandler handler;

    network::HttpResult::HeadersType headers{{"x-ld-fd-fallback", "true"}};
    network::HttpResult res{400, std::nullopt, std::move(headers)};

    auto result = HandleFDv2PollResponse(res, &handler, logger, "test");

    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Interrupted>(result.value));
    EXPECT_TRUE(result.fdv1_fallback);
}

TEST(HandleFDv2PollResponseTest, TerminalErrorPropagatesFlag) {
    auto logger = MakeNullLogger();
    FDv2ProtocolHandler handler;

    network::HttpResult::HeadersType headers{{"X-LD-FD-Fallback", "true"}};
    network::HttpResult res{401, std::nullopt, std::move(headers)};

    auto result = HandleFDv2PollResponse(res, &handler, logger, "test");

    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::TerminalError>(result.value));
    EXPECT_TRUE(result.fdv1_fallback);
}

TEST(HandleFDv2PollResponseTest, OkWithMissingBodyPropagatesFlag) {
    auto logger = MakeNullLogger();
    FDv2ProtocolHandler handler;

    network::HttpResult::HeadersType headers{{"X-LD-FD-Fallback", "true"}};
    network::HttpResult res{200, std::nullopt, std::move(headers)};

    auto result = HandleFDv2PollResponse(res, &handler, logger, "test");

    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Interrupted>(result.value));
    EXPECT_TRUE(result.fdv1_fallback);
}

TEST(HandleFDv2PollResponseTest, HeaderValueIsCaseInsensitive) {
    auto logger = MakeNullLogger();
    FDv2ProtocolHandler handler;

    network::HttpResult::HeadersType headers{{"X-LD-FD-Fallback", "TRUE"}};
    network::HttpResult res{304, std::nullopt, std::move(headers)};

    auto result = HandleFDv2PollResponse(res, &handler, logger, "test");

    EXPECT_TRUE(result.fdv1_fallback);
}

TEST(HandleFDv2PollResponseTest, HeaderValueOtherThanTrueDoesNotSetFlag) {
    auto logger = MakeNullLogger();
    FDv2ProtocolHandler handler;

    network::HttpResult::HeadersType headers{{"X-LD-FD-Fallback", "false"}};
    network::HttpResult res{304, std::nullopt, std::move(headers)};

    auto result = HandleFDv2PollResponse(res, &handler, logger, "test");

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
