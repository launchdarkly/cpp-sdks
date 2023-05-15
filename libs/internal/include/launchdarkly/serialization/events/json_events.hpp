#pragma once

#include <boost/json.hpp>

#include <launchdarkly/events/events.hpp>
#include <launchdarkly/events/summarizer.hpp>

namespace launchdarkly::events::client {
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
}  // namespace launchdarkly::events::client

namespace launchdarkly::events {

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

namespace launchdarkly::events {

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                Summarizer::State const& state);
void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                Summarizer const& summary);
}  // namespace launchdarkly::events
