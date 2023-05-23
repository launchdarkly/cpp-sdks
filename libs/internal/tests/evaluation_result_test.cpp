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
    auto evaluation_result =
        boost::json::value_to<tl::expected<EvaluationResult, JsonError>>(
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
    EXPECT_EQ(12, evaluation_result.value().Version());
    EXPECT_EQ(24, evaluation_result.value().FlagVersion());
    EXPECT_TRUE(evaluation_result.value().TrackEvents());
    EXPECT_TRUE(evaluation_result.value().TrackReason());
    EXPECT_EQ(std::chrono::system_clock::time_point{std::chrono::milliseconds{
                  1680555761}},
              evaluation_result.value().DebugEventsUntilDate());
    EXPECT_EQ(
        42,
        evaluation_result.value().Detail().Value().AsObject()["item"].AsInt());
    EXPECT_EQ(84, evaluation_result.value().Detail().VariationIndex());
    EXPECT_EQ(EvaluationReason::Kind::kOff,
              evaluation_result.value().Detail().Reason()->get().Kind());
    EXPECT_EQ(EvaluationReason::ErrorKind::kMalformedFlag,
              evaluation_result.value().Detail().Reason()->get().ErrorKind());
    EXPECT_EQ(12,
              evaluation_result.value().Detail().Reason()->get().RuleIndex());
    EXPECT_EQ("RULE_ID",
              evaluation_result.value().Detail().Reason()->get().RuleId());
    EXPECT_EQ(
        "PREREQ_KEY",
        evaluation_result.value().Detail().Reason()->get().PrerequisiteKey());
    EXPECT_EQ("STORE_ERROR",
        evaluation_result.value().Detail().Reason()->get().BigSegmentStatus());
    EXPECT_TRUE(
        evaluation_result.value().Detail().Reason()->get().InExperiment());
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
    auto evaluation_result =
        boost::json::value_to<tl::expected<EvaluationResult, JsonError>>(
            boost::json::parse("{"
                               "\"version\": 12,"
                               "\"value\": {\"item\": 42}"
                               "}"));

    EXPECT_EQ(12, evaluation_result.value().Version());
    EXPECT_EQ(std::nullopt, evaluation_result.value().FlagVersion());
    EXPECT_FALSE(evaluation_result.value().TrackEvents());
    EXPECT_FALSE(evaluation_result.value().TrackReason());
    EXPECT_EQ(std::nullopt, evaluation_result.value().DebugEventsUntilDate());
    EXPECT_EQ(
        42,
        evaluation_result.value().Detail().Value().AsObject()["item"].AsInt());
    EXPECT_EQ(std::nullopt,
              evaluation_result.value().Detail().VariationIndex());
    EXPECT_EQ(std::nullopt, evaluation_result.value().Detail().Reason());
}

TEST(EvaluationResultTests, FromMapOfResults) {
    auto results = boost::json::value_to<
        std::map<std::string, tl::expected<EvaluationResult, JsonError>>>(
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

    EXPECT_TRUE(results.at("flagA").value().Detail().Value().AsBool());
    EXPECT_FALSE(results.at("flagB").value().Detail().Value().AsBool());
}

TEST(EvaluationResultTests, NoResultFieldsJson) {
    auto results =
        boost::json::value_to<tl::expected<EvaluationResult, JsonError>>(
            boost::json::parse("{}"));

    EXPECT_FALSE(results.has_value());
    EXPECT_EQ(JsonError::kSchemaFailure, results.error());
}

TEST(EvaluationResultTests, VersionWrongTypeJson) {
    auto results =
        boost::json::value_to<tl::expected<EvaluationResult, JsonError>>(
            boost::json::parse("{\"version\": \"apple\"}"));

    EXPECT_FALSE(results.has_value());
    EXPECT_EQ(JsonError::kSchemaFailure, results.error());
}

// NOLINTEND bugprone-unchecked-optional-access
