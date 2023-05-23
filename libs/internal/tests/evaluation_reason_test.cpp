#include <gtest/gtest.h>

#include <boost/json.hpp>

#include "launchdarkly/data/evaluation_reason.hpp"
#include "launchdarkly/serialization/json_evaluation_reason.hpp"

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

    EXPECT_EQ(EvaluationReason::Kind::kOff, reason.value().Kind());
    EXPECT_EQ(EvaluationReason::ErrorKind::kMalformedFlag,
              reason.value().ErrorKind());
    EXPECT_EQ(12, reason.value().RuleIndex());
    EXPECT_EQ("RULE_ID", reason.value().RuleId());
    EXPECT_EQ("PREREQ_KEY", reason.value().PrerequisiteKey());
    EXPECT_EQ("STORE_ERROR", reason.value().BigSegmentStatus());
    EXPECT_TRUE(reason.value().InExperiment());
}

TEST(EvaluationReasonsTests, FromMinimalJson) {
    auto reason =
        boost::json::value_to<tl::expected<EvaluationReason, JsonError>>(
            boost::json::parse("{"
                               "\"kind\":\"RULE_MATCH\""
                               "}"));

    EXPECT_EQ(EvaluationReason::Kind::kRuleMatch, reason.value().Kind());
    EXPECT_EQ(std::nullopt, reason.value().ErrorKind());
    EXPECT_EQ(std::nullopt, reason.value().RuleIndex());
    EXPECT_EQ(std::nullopt, reason.value().RuleId());
    EXPECT_EQ(std::nullopt, reason.value().PrerequisiteKey());
    EXPECT_EQ(std::nullopt, reason.value().BigSegmentStatus());
    EXPECT_FALSE(reason.value().InExperiment());
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
