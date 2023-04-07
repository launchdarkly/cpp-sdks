#include <gtest/gtest.h>

#include <boost/json.hpp>

#include "context_builder.hpp"
#include "events/events.hpp"

#include "serialization/events/json_events.hpp"

namespace launchdarkly::events {

TEST(EventSerialization, Basic) {
    auto e = events::FeatureEvent{
        events::BaseEvent{
            .context =
                launchdarkly::ContextBuilder().kind("org", "ld").build()},
        "key",
        "value",
        2,
        "default",
        EvaluationReason("foo", std::nullopt, std::nullopt, std::nullopt,
                         std::nullopt, false, std::nullopt),
        17};

    auto event = boost::json::value_from(e);

    auto result = boost::json::parse(
        "{\"kind\":\"feature\",\"key\":\"key\",\"version\":17,\"variation\":2,"
        "\"value\":\"value\",\"reason\":{\"kind\":\"foo\"},\"default\":"
        "\"default\"}");

    ASSERT_EQ(result, event);
}
}  // namespace launchdarkly::events
