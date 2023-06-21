#include <gtest/gtest.h>

#include <boost/json.hpp>

#include <launchdarkly/data/evaluation_result.hpp>
#include <launchdarkly/serialization/json_evaluation_result.hpp>

using launchdarkly::EvaluationReason;
using launchdarkly::EvaluationResult;
using launchdarkly::JsonError;

// NOLINTBEGIN bugprone-unchecked-optional-access
// In the tests I do not care to check it.

using launchdarkly::Value;

TEST(EvaluationResultTests, FromJsonAllFields) {
    auto evaluation_result = boost::json::value_to<
        tl::expected<std::optional<EvaluationResult>, JsonError>>(
        boost::json::parse("{"
                           "\"version\": 12,"
                           "\"flagVersion\": 24,"
                           "\"trackEvents\": true,"
                           "\"trackReason\": true,"
                           "\"debugEventsUntilDate\": 1680555761,"
                           "\"value\": {\"item\": 42},"
                           "\"variation\": 84,"
                           "\"reason\": {"
                           "\"kind\":\"OFF\","
                           "\"errorKind\":\"MALFORMED_FLAG\","
                           "\"ruleIndex\":12,"
                           "\"ruleId\":\"RULE_ID\","
                           "\"prerequisiteKey\":\"PREREQ_KEY\","
                           "\"inExperiment\":true,"
                           "\"bigSegmentStatus\":\"STORE_ERROR\""
                           "}"
                           "}"));

    EXPECT_TRUE(evaluation_result.has_value());
    auto const& val = evaluation_result.value();
    EXPECT_TRUE(val.has_value());

    EXPECT_EQ(12, val->Version());
    EXPECT_EQ(24, val->FlagVersion());
    EXPECT_TRUE(val->TrackEvents());
    EXPECT_TRUE(val->TrackReason());
    EXPECT_EQ(std::chrono::system_clock::time_point{std::chrono::milliseconds{
                  1680555761}},
              val->DebugEventsUntilDate());
    EXPECT_EQ(42, val->Detail().Value().AsObject()["item"].AsInt());
    EXPECT_EQ(84, val->Detail().VariationIndex());
    EXPECT_EQ(EvaluationReason::Kind::kOff,
              val->Detail().Reason()->get().Kind());
    EXPECT_EQ(EvaluationReason::ErrorKind::kMalformedFlag,
              val->Detail().Reason()->get().ErrorKind());
    EXPECT_EQ(12, val->Detail().Reason()->get().RuleIndex());
    EXPECT_EQ("RULE_ID", val->Detail().Reason()->get().RuleId());
    EXPECT_EQ("PREREQ_KEY", val->Detail().Reason()->get().PrerequisiteKey());
    EXPECT_EQ("STORE_ERROR", val->Detail().Reason()->get().BigSegmentStatus());
    EXPECT_TRUE(val->Detail().Reason()->get().InExperiment());
}

TEST(EvaluationResultTests, ToJsonAllFields) {
    EvaluationReason reason(EvaluationReason::Kind::kOff,
                            EvaluationReason::ErrorKind::kMalformedFlag, 12,
                            "RULE_ID", "PREREQ_KEY", true, "STORE_ERROR");
    launchdarkly::EvaluationDetailInternal detail(
        Value(std::map<std::string, Value>{{"item", "a"}}), 84, reason);
    EvaluationResult result(12, 24, true, true,
                            std::chrono::system_clock::time_point{
                                std::chrono::milliseconds{1680555761}},
                            detail);

    auto res = boost::json::serialize(boost::json::value_from(result));
    // Strictly speaking the serialized JSON order could change.
    // In that case, then this test should be updated to check the fields of the
    // boost json value.
    EXPECT_EQ(
        "{\"version\":12,\"flagVersion\":24,\"trackEvents\":true,"
        "\"trackReason\":true,\"debugEventsUntilDate\":1680555761,\"value\":{"
        "\"item\":\"a\"},\"variationIndex\":84,\"reason\":{\"kind\":\"OFF\","
        "\"errorKind\":\"MALFORMED_FLAG\",\"bigSegmentStatus\":\"STORE_ERROR\","
        "\"ruleId\":\"RULE_ID\",\"ruleIndex\":12,\"inExperiment\":true,"
        "\"prerequisiteKey\":\"PREREQ_KEY\"}}",
        res);
}

TEST(EvaluationResultTests, FromJsonMinimalFields) {
    auto evaluation_result = boost::json::value_to<
        tl::expected<std::optional<EvaluationResult>, JsonError>>(
        boost::json::parse("{"
                           "\"version\": 12,"
                           "\"value\": {\"item\": 42}"
                           "}"));

    auto const& value = evaluation_result.value();
    EXPECT_EQ(12, value->Version());
    EXPECT_EQ(std::nullopt, value->FlagVersion());
    EXPECT_FALSE(value->TrackEvents());
    EXPECT_FALSE(value->TrackReason());
    EXPECT_EQ(std::nullopt, value->DebugEventsUntilDate());
    EXPECT_EQ(42, value->Detail().Value().AsObject()["item"].AsInt());
    EXPECT_EQ(std::nullopt, value->Detail().VariationIndex());
    EXPECT_EQ(std::nullopt, value->Detail().Reason());
}

TEST(EvaluationResultTests, FromMapOfResults) {
    auto results = boost::json::value_to<std::map<
        std::string, tl::expected<std::optional<EvaluationResult>, JsonError>>>(
        boost::json::parse("{"
                           "\"flagA\":{"
                           "\"version\": 12,"
                           "\"value\": true"
                           "},"
                           "\"flagB\":{"
                           "\"version\": 12,"
                           "\"value\": false"
                           "}"
                           "}"));
    EXPECT_TRUE(results.at("flagA").value().value().Detail().Value().AsBool());
    EXPECT_FALSE(results.at("flagB").value().value().Detail().Value().AsBool());
}

TEST(EvaluationResultTests, NoResultFieldsJson) {
    auto results = boost::json::value_to<
        tl::expected<std::optional<EvaluationResult>, JsonError>>(
        boost::json::parse("{}"));

    EXPECT_TRUE(results);
    auto const& val = results.value();
    EXPECT_FALSE(val);
}

TEST(EvaluationResultTests, VersionWrongTypeJson) {
    auto results = boost::json::value_to<
        tl::expected<std::optional<EvaluationResult>, JsonError>>(
        boost::json::parse("{\"version\": \"apple\"}"));

    EXPECT_FALSE(results);
    EXPECT_EQ(JsonError::kSchemaFailure, results.error());
}

// NOLINTEND bugprone-unchecked-optional-access
