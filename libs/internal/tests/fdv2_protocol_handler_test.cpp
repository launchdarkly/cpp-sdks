#include <gtest/gtest.h>

#include <launchdarkly/fdv2_protocol_handler.hpp>

#include <boost/json.hpp>

using namespace launchdarkly;

// Minimal valid flag JSON accepted by the existing Flag deserializer.
static char const* const kFlagJson =
    R"({"key":"my-flag","on":true,"fallthrough":{"variation":0},)"
    R"("variations":[true,false],"version":1})";

// Minimal valid segment JSON accepted by the existing Segment deserializer.
static char const* const kSegmentJson =
    R"({"key":"my-seg","version":2,"rules":[],"included":[],"excluded":[]})";

// Build a server-intent event data value.
static boost::json::value MakeServerIntent(std::string const& intent_code) {
    return boost::json::parse(
        R"({"payloads":[{"id":"p1","target":1,"intentCode":")" + intent_code +
        R"("}]})");
}

static boost::json::value MakePutObject(std::string const& kind,
                                        std::string const& key,
                                        std::string const& object_json) {
    return boost::json::parse(R"({"version":1,"kind":")" + kind +
                              R"(","key":")" + key +
                              R"(","object":)" + object_json + "}");
}

static boost::json::value MakeDeleteObject(std::string const& kind,
                                           std::string const& key,
                                           int version) {
    return boost::json::parse(R"({"version":)" + std::to_string(version) +
                              R"(,"kind":")" + kind + R"(","key":")" + key +
                              R"("})");
}

static boost::json::value MakePayloadTransferred(std::string const& state,
                                                 int version) {
    return boost::json::parse(R"({"state":")" + state + R"(","version":)" +
                              std::to_string(version) + "}");
}

// ============================================================================
// kNone intent
// ============================================================================

TEST(FDv2ProtocolHandlerTest, NoneIntentEmitsEmptyChangeSetImmediately) {
    FDv2ProtocolHandler handler;

    auto result = handler.HandleEvent("server-intent", MakeServerIntent("none"));

    auto* cs = std::get_if<data_model::FDv2ChangeSet>(&result);
    ASSERT_NE(cs, nullptr);
    EXPECT_EQ(cs->type, data_model::FDv2ChangeSet::Type::kNone);
    EXPECT_TRUE(cs->changes.empty());
    EXPECT_FALSE(cs->selector.value.has_value());
}

// ============================================================================
// kTransferFull intent
// ============================================================================

TEST(FDv2ProtocolHandlerTest, FullIntentEmitsChangeSetOnPayloadTransferred) {
    FDv2ProtocolHandler handler;

    auto r1 = handler.HandleEvent("server-intent", MakeServerIntent("xfer-full"));
    EXPECT_TRUE(std::holds_alternative<std::monostate>(r1));

    auto r2 = handler.HandleEvent(
        "put-object", MakePutObject("flag", "my-flag", kFlagJson));
    EXPECT_TRUE(std::holds_alternative<std::monostate>(r2));

    auto r3 = handler.HandleEvent(
        "payload-transferred", MakePayloadTransferred("state-abc", 7));

    auto* cs = std::get_if<data_model::FDv2ChangeSet>(&r3);
    ASSERT_NE(cs, nullptr);
    EXPECT_EQ(cs->type, data_model::FDv2ChangeSet::Type::kFull);
    EXPECT_EQ(cs->changes.size(), 1u);
    EXPECT_EQ(cs->changes[0].key, "my-flag");
    ASSERT_TRUE(cs->selector.value.has_value());
    EXPECT_EQ(cs->selector.value->state, "state-abc");
    EXPECT_EQ(cs->selector.value->version, 7);
}

TEST(FDv2ProtocolHandlerTest, FullIntentAccumulatesMultipleObjects) {
    FDv2ProtocolHandler handler;

    handler.HandleEvent("server-intent", MakeServerIntent("xfer-full"));
    handler.HandleEvent("put-object",
                        MakePutObject("flag", "flag-1", kFlagJson));
    handler.HandleEvent("put-object",
                        MakePutObject("flag", "flag-2", kFlagJson));
    handler.HandleEvent("delete-object", MakeDeleteObject("segment", "seg-1", 5));

    auto result = handler.HandleEvent(
        "payload-transferred", MakePayloadTransferred("s", 1));

    auto* cs = std::get_if<data_model::FDv2ChangeSet>(&result);
    ASSERT_NE(cs, nullptr);
    EXPECT_EQ(cs->type, data_model::FDv2ChangeSet::Type::kFull);
    EXPECT_EQ(cs->changes.size(), 3u);
}

TEST(FDv2ProtocolHandlerTest, SecondServerIntentMidTransferDiscardsAccumulatedChanges) {
    FDv2ProtocolHandler handler;

    // Start a full transfer and accumulate a change.
    handler.HandleEvent("server-intent", MakeServerIntent("xfer-full"));
    handler.HandleEvent("put-object",
                        MakePutObject("flag", "flag-1", kFlagJson));

    // A second server-intent arrives before payload-transferred.
    // The first transfer's accumulated changes should be discarded.
    auto r = handler.HandleEvent("server-intent", MakeServerIntent("xfer-changes"));
    EXPECT_TRUE(std::holds_alternative<std::monostate>(r));

    // Only the flag from the second transfer should appear in the changeset.
    handler.HandleEvent("put-object",
                        MakePutObject("flag", "flag-2", kFlagJson));
    auto result = handler.HandleEvent(
        "payload-transferred", MakePayloadTransferred("state-new", 2));

    auto* cs = std::get_if<data_model::FDv2ChangeSet>(&result);
    ASSERT_NE(cs, nullptr);
    EXPECT_EQ(cs->type, data_model::FDv2ChangeSet::Type::kPartial);
    ASSERT_EQ(cs->changes.size(), 1u);
    EXPECT_EQ(cs->changes[0].key, "flag-2");
}

// ============================================================================
// kTransferChanges intent
// ============================================================================

TEST(FDv2ProtocolHandlerTest, PartialIntentEmitsPartialChangeSet) {
    FDv2ProtocolHandler handler;

    handler.HandleEvent("server-intent", MakeServerIntent("xfer-changes"));
    handler.HandleEvent("put-object",
                        MakePutObject("segment", "my-seg", kSegmentJson));

    auto result = handler.HandleEvent(
        "payload-transferred", MakePayloadTransferred("state-xyz", 3));

    auto* cs = std::get_if<data_model::FDv2ChangeSet>(&result);
    ASSERT_NE(cs, nullptr);
    EXPECT_EQ(cs->type, data_model::FDv2ChangeSet::Type::kPartial);
    EXPECT_EQ(cs->changes.size(), 1u);
    EXPECT_EQ(cs->changes[0].key, "my-seg");
    ASSERT_TRUE(cs->selector.value.has_value());
    EXPECT_EQ(cs->selector.value->state, "state-xyz");
}

// ============================================================================
// Unknown kind in put-object → silently skipped
// ============================================================================

TEST(FDv2ProtocolHandlerTest, UnknownKindInPutObjectIsSilentlySkipped) {
    FDv2ProtocolHandler handler;

    handler.HandleEvent("server-intent", MakeServerIntent("xfer-full"));
    handler.HandleEvent("put-object",
                        MakePutObject("experiment", "exp-1", R"({"key":"exp-1","version":1})"));
    handler.HandleEvent("put-object",
                        MakePutObject("flag", "my-flag", kFlagJson));

    auto result = handler.HandleEvent(
        "payload-transferred", MakePayloadTransferred("s", 1));

    auto* cs = std::get_if<data_model::FDv2ChangeSet>(&result);
    ASSERT_NE(cs, nullptr);
    // Only the known kind (flag) should appear.
    EXPECT_EQ(cs->changes.size(), 1u);
    EXPECT_EQ(cs->changes[0].key, "my-flag");
}

TEST(FDv2ProtocolHandlerTest, UnknownKindInDeleteObjectIsSilentlySkipped) {
    FDv2ProtocolHandler handler;

    handler.HandleEvent("server-intent", MakeServerIntent("xfer-full"));
    handler.HandleEvent("delete-object", MakeDeleteObject("experiment", "exp-1", 1));
    handler.HandleEvent("delete-object", MakeDeleteObject("flag", "my-flag", 2));

    auto result = handler.HandleEvent(
        "payload-transferred", MakePayloadTransferred("s", 1));

    auto* cs = std::get_if<data_model::FDv2ChangeSet>(&result);
    ASSERT_NE(cs, nullptr);
    // Only the known kind (flag) should appear.
    EXPECT_EQ(cs->changes.size(), 1u);
    EXPECT_EQ(cs->changes[0].key, "my-flag");
}

// ============================================================================
// error event → discard accumulated data, return Error::kServerError
// ============================================================================

TEST(FDv2ProtocolHandlerTest, ErrorEventDiscardsAccumulatedDataAndReturnsError) {
    FDv2ProtocolHandler handler;

    handler.HandleEvent("server-intent", MakeServerIntent("xfer-full"));
    handler.HandleEvent("put-object",
                        MakePutObject("flag", "my-flag", kFlagJson));

    auto result = handler.HandleEvent(
        "error",
        boost::json::parse(R"({"reason":"something went wrong"})"));

    auto* err = std::get_if<FDv2ProtocolHandler::Error>(&result);
    ASSERT_NE(err, nullptr);
    EXPECT_EQ(err->kind, FDv2ProtocolHandler::Error::Kind::kServerError);
    ASSERT_TRUE(err->server_error.has_value());
    EXPECT_EQ(err->server_error->reason, "something went wrong");
    EXPECT_FALSE(err->server_error->id.has_value());

    // After the error the handler is reset. A subsequent full transfer should
    // produce an empty changeset (no leftover data from before the error).
    handler.HandleEvent("server-intent", MakeServerIntent("xfer-full"));
    auto result2 = handler.HandleEvent(
        "payload-transferred", MakePayloadTransferred("s", 1));

    auto* cs = std::get_if<data_model::FDv2ChangeSet>(&result2);
    ASSERT_NE(cs, nullptr);
    EXPECT_TRUE(cs->changes.empty());
}

TEST(FDv2ProtocolHandlerTest, ErrorEventWithIdSetsServerId) {
    FDv2ProtocolHandler handler;

    auto result = handler.HandleEvent(
        "error",
        boost::json::parse(R"({"id":"payload-123","reason":"overloaded"})"));

    auto* err = std::get_if<FDv2ProtocolHandler::Error>(&result);
    ASSERT_NE(err, nullptr);
    EXPECT_EQ(err->kind, FDv2ProtocolHandler::Error::Kind::kServerError);
    ASSERT_TRUE(err->server_error.has_value());
    ASSERT_TRUE(err->server_error->id.has_value());
    EXPECT_EQ(*err->server_error->id, "payload-123");
    EXPECT_EQ(err->server_error->reason, "overloaded");
}

TEST(FDv2ProtocolHandlerTest, MalformedPutObjectReturnsJsonError) {
    FDv2ProtocolHandler handler;

    handler.HandleEvent("server-intent", MakeServerIntent("xfer-full"));

    // 'object' field is missing required flag fields — deserialisation fails.
    auto result = handler.HandleEvent(
        "put-object",
        boost::json::parse(
            R"({"version":1,"kind":"flag","key":"f","object":{}})"));

    auto* err = std::get_if<FDv2ProtocolHandler::Error>(&result);
    ASSERT_NE(err, nullptr);
    EXPECT_EQ(err->kind, FDv2ProtocolHandler::Error::Kind::kJsonError);
    EXPECT_TRUE(err->json_error.has_value());
}

// ============================================================================
// goodbye event → return Goodbye
// ============================================================================

TEST(FDv2ProtocolHandlerTest, GoodbyeEventReturnsGoodbye) {
    FDv2ProtocolHandler handler;

    auto result = handler.HandleEvent(
        "goodbye",
        boost::json::parse(R"({"reason":"shutting down"})"));

    auto* gb = std::get_if<Goodbye>(&result);
    ASSERT_NE(gb, nullptr);
    ASSERT_TRUE(gb->reason.has_value());
    EXPECT_EQ(*gb->reason, "shutting down");
}

TEST(FDv2ProtocolHandlerTest, GoodbyeWithoutReasonReturnsGoodbye) {
    FDv2ProtocolHandler handler;

    auto result = handler.HandleEvent("goodbye", boost::json::parse(R"({})"));

    auto* gb = std::get_if<Goodbye>(&result);
    ASSERT_NE(gb, nullptr);
    EXPECT_FALSE(gb->reason.has_value());
}

// ============================================================================
// heartbeat → no-op
// ============================================================================

TEST(FDv2ProtocolHandlerTest, HeartbeatReturnsMonostate) {
    FDv2ProtocolHandler handler;

    auto result =
        handler.HandleEvent("heartbeat", boost::json::parse(R"({})"));
    EXPECT_TRUE(std::holds_alternative<std::monostate>(result));
}

// ============================================================================
// Unrecognized event type → no-op
// ============================================================================

TEST(FDv2ProtocolHandlerTest, UnknownEventTypeReturnsMonostate) {
    FDv2ProtocolHandler handler;

    auto result =
        handler.HandleEvent("future-event-type", boost::json::parse(R"({})"));
    EXPECT_TRUE(std::holds_alternative<std::monostate>(result));
}

// ============================================================================
// put-object and delete-object before server-intent are ignored
// ============================================================================

TEST(FDv2ProtocolHandlerTest, PutBeforeServerIntentIsIgnored) {
    FDv2ProtocolHandler handler;

    auto r1 = handler.HandleEvent("put-object",
                                  MakePutObject("flag", "my-flag", kFlagJson));
    EXPECT_TRUE(std::holds_alternative<std::monostate>(r1));

    handler.HandleEvent("server-intent", MakeServerIntent("xfer-full"));
    auto result = handler.HandleEvent(
        "payload-transferred", MakePayloadTransferred("s", 1));

    auto* cs = std::get_if<data_model::FDv2ChangeSet>(&result);
    ASSERT_NE(cs, nullptr);
    EXPECT_TRUE(cs->changes.empty());
}

// ============================================================================
// Reset clears accumulated state
// ============================================================================

TEST(FDv2ProtocolHandlerTest, ResetClearsState) {
    FDv2ProtocolHandler handler;

    handler.HandleEvent("server-intent", MakeServerIntent("xfer-full"));
    handler.HandleEvent("put-object",
                        MakePutObject("flag", "my-flag", kFlagJson));
    handler.Reset();

    // After reset, payload-transferred with no prior server-intent produces
    // a full changeset with no changes.
    handler.HandleEvent("server-intent", MakeServerIntent("xfer-full"));
    auto result = handler.HandleEvent(
        "payload-transferred", MakePayloadTransferred("s", 1));

    auto* cs = std::get_if<data_model::FDv2ChangeSet>(&result);
    ASSERT_NE(cs, nullptr);
    EXPECT_TRUE(cs->changes.empty());
}

TEST(FDv2ProtocolHandlerTest, PayloadTransferredWithoutServerIntentIsError) {
    FDv2ProtocolHandler handler;

    auto result = handler.HandleEvent(
        "payload-transferred", MakePayloadTransferred("s", 1));

    auto* err = std::get_if<FDv2ProtocolHandler::Error>(&result);
    ASSERT_NE(err, nullptr);
    EXPECT_EQ(err->kind, FDv2ProtocolHandler::Error::Kind::kProtocolError);
}
