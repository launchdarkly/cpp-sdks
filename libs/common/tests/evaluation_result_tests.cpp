#include <gtest/gtest.h>

#include <boost/json.hpp>

#include "data/evaluation_result.hpp"

using launchdarkly::EvaluationResult;

TEST(EvaluationResultTests, FromJsonAllFields) {
    auto evaluation_result = boost::json::value_to<EvaluationResult>(
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
                           "\"errorKind\":\"ERROR_KIND\","
                           "\"ruleIndex\":12,"
                           "\"ruleId\":\"RULE_ID\","
                           "\"prerequisiteKey\":\"PREREQ_KEY\","
                           "\"inExperiment\":true,"
                           "\"bigSegmentStatus\":\"STORE_ERROR\""
                           "}"
                           "}"));

    EXPECT_EQ(12, evaluation_result.version());
    EXPECT_EQ(24, evaluation_result.flag_version());
    EXPECT_TRUE(evaluation_result.track_events());
    EXPECT_TRUE(evaluation_result.track_reason());
    EXPECT_EQ(1680555761, evaluation_result.debug_events_until_date());
    EXPECT_EQ(42,
              evaluation_result.detail().value().as_object()["item"].as_int());
    EXPECT_EQ(84, evaluation_result.detail().variation_index());
    EXPECT_EQ("OFF", evaluation_result.detail().reason()->get().kind());
    EXPECT_EQ("ERROR_KIND",
              evaluation_result.detail().reason()->get().error_kind());
    EXPECT_EQ(12, evaluation_result.detail().reason()->get().rule_index());
    EXPECT_EQ("RULE_ID", evaluation_result.detail().reason()->get().rule_id());
    EXPECT_EQ("PREREQ_KEY",
              evaluation_result.detail().reason()->get().prerequisite_key());
    EXPECT_EQ("STORE_ERROR",
              evaluation_result.detail().reason()->get().big_segment_status());
    EXPECT_TRUE(evaluation_result.detail().reason()->get().in_experiment());
}

TEST(EvaluationResultTests, FromJsonMinimalFields) {
    auto evaluation_result = boost::json::value_to<EvaluationResult>(
        boost::json::parse("{"
                           "\"version\": 12,"
                           "\"value\": {\"item\": 42}"
                           "}"));

    EXPECT_EQ(12, evaluation_result.version());
    EXPECT_EQ(std::nullopt, evaluation_result.flag_version());
    EXPECT_FALSE(evaluation_result.track_events());
    EXPECT_FALSE(evaluation_result.track_reason());
    EXPECT_EQ(std::nullopt, evaluation_result.debug_events_until_date());
    EXPECT_EQ(42,
              evaluation_result.detail().value().as_object()["item"].as_int());
    EXPECT_EQ(std::nullopt, evaluation_result.detail().variation_index());
    EXPECT_EQ(std::nullopt, evaluation_result.detail().reason());
}

TEST(EvaluationResultTests, FromMapOfResults) {
    auto results =
        boost::json::value_to<std::map<std::string, EvaluationResult>>(
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

    EXPECT_TRUE(results.at("flagA").detail().value().as_bool());
    EXPECT_FALSE(results.at("flagB").detail().value().as_bool());
}
