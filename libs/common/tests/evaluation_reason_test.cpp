#include <gtest/gtest.h>

#include <boost/json.hpp>

#include "data/evaluation_reason.hpp"
#include "serialization/json_evaluation_reason.hpp"

using launchdarkly::EvaluationReason;

TEST(EvaluationReasonsTests, FromJsonAllFields) {
    auto reason = boost::json::value_to<tl::expected<EvaluationReason, JsonError>>(
        boost::json::parse("{"
                           "\"kind\":\"OFF\","
                           "\"errorKind\":\"ERROR_KIND\","
                           "\"ruleIndex\":12,"
                           "\"ruleId\":\"RULE_ID\","
                           "\"prerequisiteKey\":\"PREREQ_KEY\","
                           "\"inExperiment\":true,"
                           "\"bigSegmentStatus\":\"STORE_ERROR\""
                           "}"));

    EXPECT_EQ("OFF", reason.value().kind());
    EXPECT_EQ("ERROR_KIND", reason.value().error_kind());
    EXPECT_EQ(12, reason.value().rule_index());
    EXPECT_EQ("RULE_ID", reason.value().rule_id());
    EXPECT_EQ("PREREQ_KEY", reason.value().prerequisite_key());
    EXPECT_EQ("STORE_ERROR", reason.value().big_segment_status());
    EXPECT_TRUE(reason.value().in_experiment());
}

TEST(EvaluationReasonsTests, FromMinimalJson) {
    auto reason = boost::json::value_to<tl::expected<EvaluationReason, JsonError>>(
        boost::json::parse("{"
                           "\"kind\":\"RULE_MATCH\""
                           "}"));

    EXPECT_EQ("RULE_MATCH", reason.value().kind());
    EXPECT_EQ(std::nullopt, reason.value().error_kind());
    EXPECT_EQ(std::nullopt, reason.value().rule_index());
    EXPECT_EQ(std::nullopt, reason.value().rule_id());
    EXPECT_EQ(std::nullopt, reason.value().prerequisite_key());
    EXPECT_EQ(std::nullopt, reason.value().big_segment_status());
    EXPECT_FALSE(reason.value().in_experiment());
}
