#pragma once

#include <boost/json.hpp>

#include "events/events.hpp"

namespace launchdarkly::events {
/**
 * Method used by boost::json for converting a launchdarkly::events::OutputEvent
 * into boost::json::value.
 */
void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                OutputEvent const& event);

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                FeatureEvent const& event);
}  // namespace launchdarkly::events
