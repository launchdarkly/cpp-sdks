#pragma once

#include <launchdarkly/events/data/events.hpp>
#include <launchdarkly/events/detail/summarizer.hpp>

#include <boost/json.hpp>

namespace launchdarkly::events::server_side {

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                IndexEvent const& event);
}  // namespace launchdarkly::events::server_side

namespace launchdarkly::events {

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                FeatureEvent const& event);

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                FeatureEventBase const& event);

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                IdentifyEvent const& event);

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                DebugEvent const& event);

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                Date const& date);

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                TrackEvent const& event);

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                OutputEvent const& event);

}  // namespace launchdarkly::events

namespace launchdarkly::events::detail {

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                Summarizer::State const& state);
void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                Summarizer const& summary);
}  // namespace launchdarkly::events::detail
