#pragma once

#include <boost/json.hpp>

#include "events/events.hpp"

namespace launchdarkly::events {

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                Date const& date);

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                IndexEvent const& date);

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                FeatureEvent const& event);

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                FeatureEventFields const& event);

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                OutIdentifyEvent const& event);

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                DebugEvent const& event);

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                CustomEvent const& event);

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                OutputEvent const& event);

}  // namespace launchdarkly::events
