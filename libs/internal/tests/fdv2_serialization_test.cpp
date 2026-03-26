#include <gtest/gtest.h>

#include <boost/json.hpp>
#include <launchdarkly/serialization/json_fdv2_events.hpp>

using namespace launchdarkly;

// ============================================================================
// IntentCode
// ============================================================================

TEST(IntentCodeTests, DeserializesNone) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<IntentCode>, JsonError>>(
        boost::json::parse(R"("none")"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value());
    ASSERT_EQ(*result.value(), IntentCode::kNone);
}

TEST(IntentCodeTests, DeserializesXferFull) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<IntentCode>, JsonError>>(
        boost::json::parse(R"("xfer-full")"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value());
    ASSERT_EQ(*result.value(), IntentCode::kTransferFull);
}

TEST(IntentCodeTests, DeserializesXferChanges) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<IntentCode>, JsonError>>(
        boost::json::parse(R"("xfer-changes")"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value());
    ASSERT_EQ(*result.value(), IntentCode::kTransferChanges);
}

TEST(IntentCodeTests, UnknownStringDeserializesToUnknown) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<IntentCode>, JsonError>>(
        boost::json::parse(R"("bogus")"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value());
    ASSERT_EQ(*result.value(), IntentCode::kUnknown);
}

TEST(IntentCodeTests, WrongTypeReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<IntentCode>, JsonError>>(
        boost::json::parse(R"(42)"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(IntentCodeTests, NullReturnsNullopt) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<IntentCode>, JsonError>>(
        boost::json::parse(R"(null)"));
    ASSERT_TRUE(result);
    ASSERT_FALSE(result.value());
}

// ============================================================================
// ServerIntentPayload
// ============================================================================

TEST(ServerIntentPayloadTests, DeserializesValidPayload) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<ServerIntentPayload>, JsonError>>(
        boost::json::parse(
            R"({"id":"abc","target":1,"intentCode":"xfer-full","reason":"initial"})"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value());
    ASSERT_EQ(result.value()->id, "abc");
    ASSERT_EQ(result.value()->target, std::int64_t{1});
    ASSERT_EQ(result.value()->intent_code, IntentCode::kTransferFull);
    ASSERT_TRUE(result.value()->reason);
    ASSERT_EQ(*result.value()->reason, "initial");
}

TEST(ServerIntentPayloadTests, WrongTypeReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<ServerIntentPayload>, JsonError>>(
        boost::json::parse(R"("not-an-object")"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(ServerIntentPayloadTests, MissingIdReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<ServerIntentPayload>, JsonError>>(
        boost::json::parse(
            R"({"target":1,"intentCode":"none","reason":"r"})"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(ServerIntentPayloadTests, MissingIntentCodeReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<ServerIntentPayload>, JsonError>>(
        boost::json::parse(R"({"id":"x","target":1,"reason":"r"})"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(ServerIntentPayloadTests, MissingTargetReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<ServerIntentPayload>, JsonError>>(
        boost::json::parse(R"({"id":"x","intentCode":"none","reason":"r"})"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(ServerIntentPayloadTests, MissingReasonSucceeds) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<ServerIntentPayload>, JsonError>>(
        boost::json::parse(R"({"id":"x","target":1,"intentCode":"none"})"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value());
    ASSERT_FALSE(result.value()->reason);
}

TEST(ServerIntentPayloadTests, NullReturnsNullopt) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<ServerIntentPayload>, JsonError>>(
        boost::json::parse(R"(null)"));
    ASSERT_TRUE(result);
    ASSERT_FALSE(result.value());
}

// ============================================================================
// ServerIntent
// ============================================================================

TEST(ServerIntentTests, DeserializesValidIntent) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<ServerIntent>, JsonError>>(
        boost::json::parse(
            R"({"payloads":[{"id":"abc","target":1,"intentCode":"xfer-full","reason":"r"}]})"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value());
    ASSERT_EQ(result.value()->payloads.size(), 1u);
    ASSERT_EQ(result.value()->payloads[0].id, "abc");
}

TEST(ServerIntentTests, DeserializesEmptyPayloads) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<ServerIntent>, JsonError>>(
        boost::json::parse(R"({"payloads":[]})"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value());
    ASSERT_TRUE(result.value()->payloads.empty());
}

TEST(ServerIntentTests, MissingPayloadsReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<ServerIntent>, JsonError>>(
        boost::json::parse(R"({})"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(ServerIntentTests, WrongTypeReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<ServerIntent>, JsonError>>(
        boost::json::parse(R"([])"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(ServerIntentTests, NullReturnsNullopt) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<ServerIntent>, JsonError>>(
        boost::json::parse(R"(null)"));
    ASSERT_TRUE(result);
    ASSERT_FALSE(result.value());
}

// ============================================================================
// PutObject
// ============================================================================

TEST(PutObjectTests, DeserializesValidPutObject) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<PutObject>, JsonError>>(
        boost::json::parse(
            R"({"version":5,"kind":"flag","key":"my-flag","object":{"on":true}})"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value());
    ASSERT_EQ(result.value()->version, std::int64_t{5});
    ASSERT_EQ(result.value()->kind, "flag");
    ASSERT_EQ(result.value()->key, "my-flag");
    ASSERT_TRUE(result.value()->object.is_object());
}

TEST(PutObjectTests, MissingObjectReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<PutObject>, JsonError>>(
        boost::json::parse(R"({"version":1,"kind":"flag","key":"k"})"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(PutObjectTests, MissingVersionReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<PutObject>, JsonError>>(
        boost::json::parse(
            R"({"kind":"flag","key":"k","object":{}})"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(PutObjectTests, MissingKindReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<PutObject>, JsonError>>(
        boost::json::parse(R"({"version":1,"key":"k","object":{}})"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(PutObjectTests, MissingKeyReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<PutObject>, JsonError>>(
        boost::json::parse(R"({"version":1,"kind":"flag","object":{}})"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(PutObjectTests, WrongTypeReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<PutObject>, JsonError>>(
        boost::json::parse(R"("string")"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(PutObjectTests, NullReturnsNullopt) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<PutObject>, JsonError>>(
        boost::json::parse(R"(null)"));
    ASSERT_TRUE(result);
    ASSERT_FALSE(result.value());
}

// ============================================================================
// DeleteObject
// ============================================================================

TEST(DeleteObjectTests, DeserializesValidDeleteObject) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<DeleteObject>, JsonError>>(
        boost::json::parse(
            R"({"version":3,"kind":"segment","key":"my-seg"})"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value());
    ASSERT_EQ(result.value()->version, std::int64_t{3});
    ASSERT_EQ(result.value()->kind, "segment");
    ASSERT_EQ(result.value()->key, "my-seg");
}

TEST(DeleteObjectTests, MissingVersionReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<DeleteObject>, JsonError>>(
        boost::json::parse(R"({"kind":"flag","key":"k"})"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(DeleteObjectTests, MissingKindReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<DeleteObject>, JsonError>>(
        boost::json::parse(R"({"version":1,"key":"k"})"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(DeleteObjectTests, MissingKeyReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<DeleteObject>, JsonError>>(
        boost::json::parse(R"({"version":1,"kind":"flag"})"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(DeleteObjectTests, WrongTypeReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<DeleteObject>, JsonError>>(
        boost::json::parse(R"([1,2,3])"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(DeleteObjectTests, NullReturnsNullopt) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<DeleteObject>, JsonError>>(
        boost::json::parse(R"(null)"));
    ASSERT_TRUE(result);
    ASSERT_FALSE(result.value());
}

// ============================================================================
// PayloadTransferred
// ============================================================================

TEST(PayloadTransferredTests, DeserializesValidPayload) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<PayloadTransferred>, JsonError>>(
        boost::json::parse(R"json({"state":"(p:abc:42)","version":42})json"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value());
    ASSERT_EQ(result.value()->state, std::string("(p:abc:42)"));
    ASSERT_EQ(result.value()->version, std::int64_t{42});
}

TEST(PayloadTransferredTests, MissingStateReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<PayloadTransferred>, JsonError>>(
        boost::json::parse(R"({"version":1})"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(PayloadTransferredTests, MissingVersionReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<PayloadTransferred>, JsonError>>(
        boost::json::parse(R"({"state":"s"})"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(PayloadTransferredTests, WrongTypeReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<PayloadTransferred>, JsonError>>(
        boost::json::parse(R"("string")"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(PayloadTransferredTests, NullReturnsNullopt) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<PayloadTransferred>, JsonError>>(
        boost::json::parse(R"(null)"));
    ASSERT_TRUE(result);
    ASSERT_FALSE(result.value());
}

// ============================================================================
// Goodbye
// ============================================================================

TEST(GoodbyeTests, DeserializesWithReason) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<Goodbye>, JsonError>>(
        boost::json::parse(R"({"reason":"shutting down"})"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value());
    ASSERT_TRUE(result.value()->reason);
    ASSERT_EQ(*result.value()->reason, "shutting down");
}

TEST(GoodbyeTests, DeserializesWithoutReason) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<Goodbye>, JsonError>>(
        boost::json::parse(R"({})"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value());
    ASSERT_FALSE(result.value()->reason);
}

TEST(GoodbyeTests, WrongTypeReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<Goodbye>, JsonError>>(
        boost::json::parse(R"("goodbye")"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(GoodbyeTests, NullReturnsNullopt) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<Goodbye>, JsonError>>(
        boost::json::parse(R"(null)"));
    ASSERT_TRUE(result);
    ASSERT_FALSE(result.value());
}

// ============================================================================
// FDv2Error
// ============================================================================

TEST(FDv2ErrorTests, DeserializesWithRequiredFieldOnly) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<FDv2Error>, JsonError>>(
        boost::json::parse(R"({"reason":"something went wrong"})"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value());
    ASSERT_EQ(result.value()->reason, "something went wrong");
    ASSERT_FALSE(result.value()->id);
}

TEST(FDv2ErrorTests, DeserializesWithAllFields) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<FDv2Error>, JsonError>>(
        boost::json::parse(R"({"reason":"oops","id":"err-42"})"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value());
    ASSERT_EQ(result.value()->reason, "oops");
    ASSERT_TRUE(result.value()->id);
    ASSERT_EQ(*result.value()->id, "err-42");
}

TEST(FDv2ErrorTests, MissingReasonReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<FDv2Error>, JsonError>>(
        boost::json::parse(R"({"id":"e1"})"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(FDv2ErrorTests, WrongTypeReturnsSchemaFailure) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<FDv2Error>, JsonError>>(
        boost::json::parse(R"([])"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(FDv2ErrorTests, NullReturnsNullopt) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<FDv2Error>, JsonError>>(
        boost::json::parse(R"(null)"));
    ASSERT_TRUE(result);
    ASSERT_FALSE(result.value());
}
