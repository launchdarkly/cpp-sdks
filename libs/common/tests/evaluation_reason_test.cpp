#include <gtest/gtest.h>

#include <boost/json.hpp>

#include "data/evaluation_reason.hpp"
#include "serialization/json_evaluation_reason.hpp"

using launchdarkly::EvaluationReason;

TEST(EvaluationReasonsTests, FromJsonAllFields) {
    auto reason = boost::json::value_to<EvaluationReason>(
        boost::json::parse("{"
                           "\"kind\":\"OFF\","
                           "\"errorKind\":\"ERROR_KIND\","
                           "\"ruleIndex\":12,"
                           "\"ruleId\":\"RULE_ID\","
                           "\"prerequisiteKey\":\"PREREQ_KEY\","
                           "\"inExperiment\":true,"
                           "\"bigSegmentStatus\":\"STORE_ERROR\""
                           "}"));

    EXPECT_EQ("OFF", reason.kind());
    EXPECT_EQ("ERROR_KIND", reason.error_kind());
    EXPECT_EQ(12, reason.rule_index());
    EXPECT_EQ("RULE_ID", reason.rule_id());
    EXPECT_EQ("PREREQ_KEY", reason.prerequisite_key());
    EXPECT_EQ("STORE_ERROR", reason.big_segment_status());
    EXPECT_TRUE(reason.in_experiment());
}

TEST(EvaluationReasonsTests, FromMinimalJson) {
    auto reason = boost::json::value_to<EvaluationReason>(
        boost::json::parse("{"
                           "\"kind\":\"RULE_MATCH\""
                           "}"));

    EXPECT_EQ("RULE_MATCH", reason.kind());
    EXPECT_EQ(std::nullopt, reason.error_kind());
    EXPECT_EQ(std::nullopt, reason.rule_index());
    EXPECT_EQ(std::nullopt, reason.rule_id());
    EXPECT_EQ(std::nullopt, reason.prerequisite_key());
    EXPECT_EQ(std::nullopt, reason.big_segment_status());
    EXPECT_FALSE(reason.in_experiment());
}
