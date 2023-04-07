#include <gtest/gtest.h>

#include <boost/json.hpp>

#include "context_builder.hpp"
#include "events/events.hpp"

#include "serialization/events/json_events.hpp"

namespace launchdarkly::events {

TEST(EventSerialization, FeatureEvent) {
    auto creation_date = std::chrono::system_clock::from_time_t({});
    auto e = events::FeatureEvent{
        {"key", creation_date, "value", 2, "default",
         EvaluationReason("foo", std::nullopt, std::nullopt, std::nullopt,
                          std::nullopt, false, std::nullopt),
         17},
        std::map<std::string, std::string>{{"foo", "bar"}}};

    auto event = boost::json::value_from(e);

    auto result = boost::json::parse(
        "{\"kind\":\"feature\",\"creationDate\":0,\"contextKeys\":{\"foo\":"
        "\"bar\"},\"key\":\"key\",\"version\":17,\"variation\":2,\"value\":"
        "\"value\",\"reason\":{\"kind\":\"foo\"},\"default\":\"default\"}");

    ASSERT_EQ(result, event);
}

TEST(EventSerialization, DebugEvent) {
    auto creation_date = std::chrono::system_clock::from_time_t({});
    auto e = events::DebugEvent{
        {"key", creation_date, "value", 2, "default",
         EvaluationReason("foo", std::nullopt, std::nullopt, std::nullopt,
                          std::nullopt, false, std::nullopt),
         17},
        ContextBuilder().kind("foo", "bar").build()};

    auto event = boost::json::value_from(e);

    auto result = boost::json::parse(
        "{\"kind\":\"feature\",\"creationDate\":0,\"key\":\"key\",\"version\":"
        "17,\"variation\":2,\"value\":\"value\",\"reason\":{\"kind\":\"foo\"},"
        "\"default\":\"default\",\"context\":{\"key\":\"bar\",\"kind\":\"foo\"}"
        "}");

    ASSERT_EQ(result, event);
}
}  // namespace launchdarkly::events
