#include <gtest/gtest.h>

#include <boost/json.hpp>

#include <launchdarkly/events/events.hpp>
#include <launchdarkly/context_builder.hpp>

#include <launchdarkly/context_filter.hpp>
#include <launchdarkly/value.hpp>
#include <launchdarkly/serialization/events/json_events.hpp>

namespace launchdarkly::events {

TEST(EventSerialization, FeatureEvent) {
    auto creation_date = std::chrono::system_clock::from_time_t({});
    auto event = events::client::FeatureEvent{
        client::FeatureEventBase(client::FeatureEventParams{
            creation_date,
            "key",
            ContextBuilder().kind("foo", "bar").build(),
            Value(42),
            Value(3),
            1,
            2,
            std::nullopt,
            false,
            std::nullopt,

        }),
        std::map<std::string, std::string>{{"foo", "bar"}}};

    auto event_json = boost::json::value_from(event);

    auto result = boost::json::parse(
        R"({"creationDate":0,"key":"key","version":1,"variation":2,"value":4.2E1,"default":3E0,"kind":"feature","contextKeys":{"foo":"bar"}})");

    ASSERT_EQ(result, event_json);
}

TEST(EventSerialization, DebugEvent) {
    auto creation_date = std::chrono::system_clock::from_time_t({});
    AttributeReference::SetType attrs;
    ContextFilter filter(false, attrs);
    auto context = ContextBuilder().kind("foo", "bar").build();
    auto event = events::client::DebugEvent{
        client::FeatureEventBase(
            client::FeatureEventBase(client::FeatureEventParams{
                creation_date,
                "key",
                ContextBuilder().kind("foo", "bar").build(),
                Value(42),
                Value(3),
                1,
                2,
                std::nullopt,
                false,
                std::nullopt,

            })),
        filter.filter(context)};

    auto event_json = boost::json::value_from(event);

    auto result = boost::json::parse(
        R"({"creationDate":0,"key":"key","version":1,"variation":2,"value":4.2E1,"default":3E0,"kind":"debug","context":{"key":"bar","kind":"foo"}})");

    ASSERT_EQ(result, event_json);
}

TEST(EventSerialization, IdentifyEvent) {
    auto creation_date = std::chrono::system_clock::from_time_t({});
    AttributeReference::SetType attrs;
    ContextFilter filter(false, attrs);
    auto event = events::client::IdentifyEvent{
        creation_date,
        filter.filter(ContextBuilder().kind("foo", "bar").build())};

    auto event_json = boost::json::value_from(event);

    auto result = boost::json::parse(
        R"({"kind":"identify","creationDate":0,"context":{"key":"bar","kind":"foo"}})");
    ASSERT_EQ(result, event_json);
}

}  // namespace launchdarkly::events
