#include <gtest/gtest.h>

#include <data_sources/fdv2/fdv2_polling_impl.hpp>

#include <launchdarkly/config/shared/defaults.hpp>
#include <launchdarkly/fdv2_protocol_handler.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/network/http_requester.hpp>

using namespace launchdarkly;
using namespace launchdarkly::client_side::data_sources;
using namespace std::chrono_literals;

// A flag-eval put-object followed by payload-transferred: the smallest
// response that yields a full data set.
static char const* const kFullTransferBody =
    R"({"events":[)"
    R"({"event":"server-intent","data":{"payloads":[)"
    R"({"id":"p1","target":1,"intentCode":"xfer-full"}]}},)"
    R"({"event":"put-object","data":{"version":7,"kind":"flag-eval",)"
    R"("key":"my-flag","object":{"value":"a","variation":1}}},)"
    R"({"event":"payload-transferred","data":{"state":"abc","version":7}})"
    R"(]})";

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

static FDv2RequestConfig MakeConfig(std::string base_url,
                                    FDv2ContextTransport transport,
                                    bool with_reasons) {
    return FDv2RequestConfig{
        std::move(base_url),
        config::shared::Defaults<config::shared::ClientSDK>::HttpProperties(),
        R"({"kind":"user","key":"user-key"})", transport, with_reasons};
}

TEST(ClientMakeFDv2PollRequestTest, EncodesTheContextIntoTheGetPath) {
    auto req = MakeFDv2PollRequest(
        MakeConfig("http://example.com", FDv2ContextTransport::kGetPath, false),
        data_model::Selector{});

    EXPECT_EQ(network::HttpMethod::kGet, req.Method());
    // The context, base64url-encoded, is the final path segment.
    EXPECT_EQ(
        "http://example.com/sdk/poll/eval/"
        "eyJraW5kIjoidXNlciIsImtleSI6InVzZXIta2V5In0=",
        req.Url());
    EXPECT_FALSE(req.Body().has_value());
}

TEST(ClientMakeFDv2PollRequestTest, EncodesTheContextInThePostBody) {
    auto req =
        MakeFDv2PollRequest(MakeConfig("http://example.com",
                                       FDv2ContextTransport::kPostBody, false),
                            data_model::Selector{});

    // POST carries the context in the body, so the path has no context segment.
    EXPECT_EQ(network::HttpMethod::kPost, req.Method());
    EXPECT_EQ("http://example.com/sdk/poll/eval", req.Url());
    ASSERT_TRUE(req.Body().has_value());
    EXPECT_EQ(R"({"kind":"user","key":"user-key"})", *req.Body());
    EXPECT_EQ("application/json",
              req.Properties().BaseHeaders().at("content-type"));
}

TEST(ClientMakeFDv2PollRequestTest, EncodesANonEmptySelectorAsTheBasis) {
    auto req = MakeFDv2PollRequest(
        MakeConfig("http://example.com", FDv2ContextTransport::kPostBody,
                   false),
        data_model::Selector{data_model::Selector::State{3, "state-3"}});

    EXPECT_EQ("http://example.com/sdk/poll/eval?basis=state-3", req.Url());
}

TEST(ClientMakeFDv2PollRequestTest, RequestsReasonsWhenConfigured) {
    auto req = MakeFDv2PollRequest(
        MakeConfig("http://example.com", FDv2ContextTransport::kPostBody, true),
        data_model::Selector{});

    EXPECT_EQ("http://example.com/sdk/poll/eval?withReasons=true", req.Url());
}

TEST(ClientMakeFDv2PollRequestTest, OmitsTheConditionalRequestValidator) {
    auto req = MakeFDv2PollRequest(
        MakeConfig("http://example.com", FDv2ContextTransport::kGetPath, false),
        data_model::Selector{data_model::Selector::State{3, "state-3"}});

    EXPECT_EQ(0u, req.Properties().BaseHeaders().count("if-none-match"));
    EXPECT_EQ(0u, req.Properties().BaseHeaders().count("If-None-Match"));
}

TEST(ClientMakeFDv2PollRequestTest, BaseWithTrailingSlashJoinsCleanly) {
    auto req =
        MakeFDv2PollRequest(MakeConfig("http://example.com/",
                                       FDv2ContextTransport::kPostBody, false),
                            data_model::Selector{});

    EXPECT_EQ("http://example.com/sdk/poll/eval", req.Url());
}

TEST(ClientMakeFDv2PollRequestTest, BaseWithSubpathJoinsCleanly) {
    auto req =
        MakeFDv2PollRequest(MakeConfig("http://example.com/relay/",
                                       FDv2ContextTransport::kPostBody, false),
                            data_model::Selector{});

    EXPECT_EQ("http://example.com/relay/sdk/poll/eval", req.Url());
}

TEST(ClientMakeFDv2PollRequestTest,
     UnparseableBaseUrlProducesAnInvalidRequest) {
    auto req = MakeFDv2PollRequest(
        MakeConfig("not a url", FDv2ContextTransport::kGetPath, false),
        data_model::Selector{});

    EXPECT_FALSE(req.Valid());
}

TEST(ClientHandleFDv2PollResponseTest, TranslatesAFullTransferToAChangeSet) {
    auto result = HandleResponse(200, kFullTransferBody, {});

    auto* change_set = std::get_if<FDv2SourceResult::ChangeSet>(&result.value);
    ASSERT_NE(nullptr, change_set);
    EXPECT_EQ(data_model::ChangeSetType::kFull, change_set->change_set.type);
    ASSERT_EQ(1u, change_set->change_set.data.size());
    EXPECT_EQ("my-flag", change_set->change_set.data[0].key);
    ASSERT_TRUE(change_set->change_set.selector.value.has_value());
    EXPECT_EQ("abc", change_set->change_set.selector.value->state);
}

TEST(ClientHandleFDv2PollResponseTest, TreatsNotModifiedAsANoneIntent) {
    auto result = HandleResponse(304, std::nullopt, {});

    // 304 carries no body and surfaces as a none changeset.
    auto* change_set = std::get_if<FDv2SourceResult::ChangeSet>(&result.value);
    ASSERT_NE(nullptr, change_set);
    EXPECT_EQ(data_model::ChangeSetType::kNone, change_set->change_set.type);
    EXPECT_TRUE(change_set->change_set.data.empty());
}

TEST(ClientHandleFDv2PollResponseTest, ReportsTheEnvironmentId) {
    auto result =
        HandleResponse(200, kFullTransferBody, {{"X-LD-EnvId", "env-1234"}});

    ASSERT_TRUE(result.environment_id.has_value());
    EXPECT_EQ("env-1234", *result.environment_id);
}

TEST(ClientHandleFDv2PollResponseTest, ReportsNoEnvironmentIdWhenAbsent) {
    auto result = HandleResponse(200, kFullTransferBody, {});

    EXPECT_FALSE(result.environment_id.has_value());
}

TEST(ClientHandleFDv2PollResponseTest, RecoverableStatusIsInterrupted) {
    auto result = HandleResponse(500, std::nullopt, {});

    // A 500 is recoverable, so it interrupts rather than terminates.
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Interrupted>(result.value));
}

TEST(ClientHandleFDv2PollResponseTest, UnrecoverableStatusIsTerminal) {
    auto result = HandleResponse(401, std::nullopt, {});

    // A 401 is not recoverable, so the source terminates.
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::TerminalError>(result.value));
}

TEST(ClientHandleFDv2PollResponseTest, NetworkErrorIsInterrupted) {
    auto logger = MakeNullLogger();
    FDv2ProtocolHandler handler;
    network::HttpResult res{std::optional<std::string>{"connection refused"}};

    auto result = HandleFDv2PollResponse(res, &handler, logger, "test");

    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Interrupted>(result.value));
    // A transport error carries no response headers, so no fallback directive.
    EXPECT_FALSE(result.fdv1_fallback.has_value());
}

TEST(ClientHandleFDv2PollResponseTest, AbandonsAnUntranslatableChangeSet) {
    // The put-object's object field is an array, not an object.
    std::string const body =
        R"({"events":[)"
        R"({"event":"server-intent","data":{"payloads":[)"
        R"({"id":"p1","target":1,"intentCode":"xfer-full"}]}},)"
        R"({"event":"put-object","data":{"version":7,"kind":"flag-eval",)"
        R"("key":"my-flag","object":["not-an-object"]}},)"
        R"({"event":"payload-transferred","data":{"state":"abc","version":7}})"
        R"(]})";

    auto result = HandleResponse(200, body, {});

    // The whole payload is abandoned, surfacing as a recoverable interruption.
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Interrupted>(result.value));
}

TEST(ClientHandleFDv2PollResponseTest, ReadsTheFDv1FallbackDirective) {
    auto result = HandleResponse(
        200, kFullTransferBody,
        {{"X-LD-FD-Fallback", "true"}, {"X-LD-FD-Fallback-TTL", "120"}});

    // The payload that arrived with the directive is still applied.
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::ChangeSet>(result.value));
    ASSERT_TRUE(result.fdv1_fallback.has_value());
    EXPECT_EQ(120s, result.fdv1_fallback->ttl);
}

TEST(ClientHandleFDv2PollResponseTest, FDv1FallbackHeaderIsCaseInsensitive) {
    auto result =
        HandleResponse(304, std::nullopt, {{"x-ld-fd-fallback", "TRUE"}});

    EXPECT_TRUE(result.fdv1_fallback.has_value());
}

TEST(ClientHandleFDv2PollResponseTest, FDv1FallbackHeaderOtherThanTrueIgnored) {
    auto result =
        HandleResponse(304, std::nullopt, {{"X-LD-FD-Fallback", "false"}});

    EXPECT_FALSE(result.fdv1_fallback.has_value());
}

TEST(ClientHandleFDv2PollResponseTest, FDv1FallbackTravelsWithATerminalError) {
    auto result =
        HandleResponse(401, std::nullopt, {{"X-LD-FD-Fallback", "true"}});

    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::TerminalError>(result.value));
    EXPECT_TRUE(result.fdv1_fallback.has_value());
}

TEST(ClientHandleFDv2PollResponseTest, GoodbyeCarriesItsOwnFallbackTtl) {
    std::string const body =
        R"({"events":[)"
        R"({"event":"goodbye","data":{"reason":"bye","protocolFallbackTTL":90}})"
        R"(]})";

    auto result = HandleResponse(
        200, body,
        {{"X-LD-FD-Fallback", "true"}, {"X-LD-FD-Fallback-TTL", "120"}});

    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Goodbye>(result.value));
    // The goodbye's own TTL (90) wins over the header's (120).
    ASSERT_TRUE(result.fdv1_fallback.has_value());
    EXPECT_EQ(90s, result.fdv1_fallback->ttl);
}
