#pragma once

#include <boost/json.hpp>

#include "events/events.hpp"

namespace launchdarkly::events {

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                OutputEvent const& event);

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                FeatureEvent const& event);

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                FeatureEventFields const& event);

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                IdentifyEvent const& event);

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                DebugEvent const& event);
}  // namespace launchdarkly::events
