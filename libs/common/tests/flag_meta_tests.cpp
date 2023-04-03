#include <gtest/gtest.h>

#include <boost/json.hpp>

#include "data/flag_meta.hpp"

using launchdarkly::FlagMeta;

TEST(FlagMetaTests, FromJsonAllFields) {
    auto flag_meta = boost::json::value_to<FlagMeta>(
        boost::json::parse("{"
                           "\"version\": 12,"
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

    EXPECT_EQ(12, flag_meta.version());
    EXPECT_TRUE(flag_meta.track_events());
    EXPECT_TRUE(flag_meta.track_reason());
    EXPECT_EQ(1680555761, flag_meta.debug_events_until_date());
    EXPECT_EQ(42, flag_meta.detail().value().as_object()["item"].as_int());
    EXPECT_EQ(84, flag_meta.detail().variation_index());
    EXPECT_EQ("OFF", flag_meta.detail().reason()->get().kind());
    EXPECT_EQ("ERROR_KIND", flag_meta.detail().reason()->get().error_kind());
    EXPECT_EQ(12, flag_meta.detail().reason()->get().rule_index());
    EXPECT_EQ("RULE_ID", flag_meta.detail().reason()->get().rule_id());
    EXPECT_EQ("PREREQ_KEY",
              flag_meta.detail().reason()->get().prerequisite_key());
    EXPECT_EQ("STORE_ERROR",
              flag_meta.detail().reason()->get().big_segment_status());
    EXPECT_TRUE(flag_meta.detail().reason()->get().in_experiment());
}

TEST(FlagMetaTests, fromJsonMinimalFields) {
    auto flag_meta = boost::json::value_to<FlagMeta>(
        boost::json::parse("{"
                           "\"version\": 12,"
                           "\"value\": {\"item\": 42}"
                           "}"));

    EXPECT_EQ(12, flag_meta.version());
    EXPECT_FALSE(flag_meta.track_events());
    EXPECT_FALSE(flag_meta.track_reason());
    EXPECT_EQ(std::nullopt, flag_meta.debug_events_until_date());
    EXPECT_EQ(42, flag_meta.detail().value().as_object()["item"].as_int());
    EXPECT_EQ(std::nullopt, flag_meta.detail().variation_index());
    EXPECT_EQ(std::nullopt, flag_meta.detail().reason());
}
