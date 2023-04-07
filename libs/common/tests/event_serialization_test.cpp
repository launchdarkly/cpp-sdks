#include <gtest/gtest.h>

#include <boost/json.hpp>

#include "context_builder.hpp"
#include "events/events.hpp"

#include "serialization/json_events.hpp"

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
        events::Reason{"reason"},
        17};

    auto event = boost::json::value_from(e);

    auto result = boost::json::parse("{}");

    ASSERT_EQ(result, event);
}
}  // namespace launchdarkly::events
