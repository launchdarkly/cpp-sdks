#include <gtest/gtest.h>

#include <boost/json.hpp>

#include "context_builder.hpp"
#include "events/events.hpp"

#include "context_filter.hpp"
#include "serialization/events/json_events.hpp"
#include "value.hpp"

namespace launchdarkly::events {

TEST(EventSerialization, FeatureEvent) {
    auto creation_date = std::chrono::system_clock::from_time_t({});
    auto e = events::client::FeatureEvent{
        {creation_date, "key", 17, 2, "value",
         EvaluationReason("foo", std::nullopt, std::nullopt, std::nullopt,
                          std::nullopt, false, std::nullopt),

         17},
        std::map<std::string, std::string>{{"foo", "bar"}}};

    auto event = boost::json::value_from(e);

    auto result = boost::json::parse(
        "{\"creationDate\":0,\"key\":\"key\",\"version\":17,\"variation\":2,"
        "\"value\":\"value\",\"reason\":{\"kind\":\"foo\"},\"default\":1.7E1,"
        "\"kind\":\"feature\",\"contextKeys\":{\"foo\":\"bar\"}}");

    ASSERT_EQ(result, event);
}

TEST(EventSerialization, DebugEvent) {
    auto creation_date = std::chrono::system_clock::from_time_t({});
    AttributeReference::SetType attrs;
    ContextFilter filter(false, attrs);
    auto e = events::client::DebugEvent{
        {creation_date, "key", 17, 2, "value",
         EvaluationReason("foo", std::nullopt, std::nullopt, std::nullopt,
                          std::nullopt, false, std::nullopt),

         17},
        filter.filter(ContextBuilder().kind("foo", "bar").build())};

    auto event = boost::json::value_from(e);

    auto result = boost::json::parse(
        "{\"creationDate\":0,\"key\":\"key\",\"version\":17,\"variation\":2,"
        "\"value\":\"value\",\"reason\":{\"kind\":\"foo\"},\"default\":1.7E1,"
        "\"kind\":\"debug\",\"context\":{\"key\":\"bar\",\"kind\":\"foo\"}}");

    ASSERT_EQ(result, event);
}

TEST(EventSerialization, IdentifyEvent) {
    auto creation_date = std::chrono::system_clock::from_time_t({});
    AttributeReference::SetType attrs;
    ContextFilter filter(false, attrs);
    auto e = events::client::IdentifyEvent{
        creation_date,
        filter.filter(ContextBuilder().kind("foo", "bar").build())};

    auto event = boost::json::value_from(e);

    auto result = boost::json::parse(
        "{\"kind\":\"identify\",\"creationDate\":0,\"context\":{\"key\":"
        "\"bar\",\"kind\":\"foo\"}}");

    ASSERT_EQ(result, event);
}

}  // namespace launchdarkly::events
