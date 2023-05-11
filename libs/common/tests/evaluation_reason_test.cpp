#include <gtest/gtest.h>

#include <boost/json.hpp>

#include "data/evaluation_reason.hpp"
#include "serialization/json_evaluation_reason.hpp"

using launchdarkly::EvaluationReason;
using launchdarkly::JsonError;

TEST(EvaluationReasonsTests, FromJsonAllFields) {
    auto reason =
        boost::json::value_to<tl::expected<EvaluationReason, JsonError>>(
            boost::json::parse("{"
                               "\"kind\":\"OFF\","
                               "\"errorKind\":\"MALFORMED_FLAG\","
                               "\"ruleIndex\":12,"
                               "\"ruleId\":\"RULE_ID\","
                               "\"prerequisiteKey\":\"PREREQ_KEY\","
                               "\"inExperiment\":true,"
                               "\"bigSegmentStatus\":\"STORE_ERROR\""
                               "}"));

    EXPECT_EQ(EvaluationReason::Kind::kOff, reason.value().kind());
    EXPECT_EQ(EvaluationReason::ErrorKind::kMalformedFlag,
              reason.value().error_kind());
    EXPECT_EQ(12, reason.value().rule_index());
    EXPECT_EQ("RULE_ID", reason.value().rule_id());
    EXPECT_EQ("PREREQ_KEY", reason.value().prerequisite_key());
    EXPECT_EQ("STORE_ERROR", reason.value().big_segment_status());
    EXPECT_TRUE(reason.value().in_experiment());
}

TEST(EvaluationReasonsTests, FromMinimalJson) {
    auto reason =
        boost::json::value_to<tl::expected<EvaluationReason, JsonError>>(
            boost::json::parse("{"
                               "\"kind\":\"RULE_MATCH\""
                               "}"));

    EXPECT_EQ(EvaluationReason::Kind::kRuleMatch, reason.value().kind());
    EXPECT_EQ(std::nullopt, reason.value().error_kind());
    EXPECT_EQ(std::nullopt, reason.value().rule_index());
    EXPECT_EQ(std::nullopt, reason.value().rule_id());
    EXPECT_EQ(std::nullopt, reason.value().prerequisite_key());
    EXPECT_EQ(std::nullopt, reason.value().big_segment_status());
    EXPECT_FALSE(reason.value().in_experiment());
}

TEST(EvaluationReasonsTests, NoReasonFieldsJson) {
    auto results =
        boost::json::value_to<tl::expected<EvaluationReason, JsonError>>(
            boost::json::parse("{}"));

    EXPECT_FALSE(results.has_value());
    EXPECT_EQ(JsonError::kSchemaFailure, results.error());
}

TEST(EvaluationReasonsTests, InvalidKindReason) {
    auto results =
        boost::json::value_to<tl::expected<EvaluationReason, JsonError>>(
            boost::json::parse("{\"kind\": 17}"));

    EXPECT_FALSE(results.has_value());
    EXPECT_EQ(JsonError::kSchemaFailure, results.error());
}
