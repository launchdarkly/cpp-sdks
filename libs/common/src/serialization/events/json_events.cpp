#include "launchdarkly/serialization/events/json_events.hpp"
#include "launchdarkly/serialization/json_context.hpp"
#include "launchdarkly/serialization/json_evaluation_reason.hpp"
#include "launchdarkly/serialization/json_value.hpp"

namespace launchdarkly::events::client {
void tag_invoke(boost::json::value_from_tag const& tag,
                boost::json::value& json_value,
                FeatureEvent const& event) {
    auto base = boost::json::value_from<FeatureEventBase const&>(event);
    base.as_object().emplace("kind", "feature");
    base.as_object().emplace("contextKeys",
                             boost::json::value_from(event.context_keys));
    json_value = std::move(base);
}

void tag_invoke(boost::json::value_from_tag const& tag,
                boost::json::value& json_value,
                DebugEvent const& event) {
    auto base = boost::json::value_from<FeatureEventBase const&>(event);
    base.as_object().emplace("kind", "debug");
    base.as_object().emplace("context", boost::json::value_from(event.context));
    json_value = std::move(base);
}

void tag_invoke(boost::json::value_from_tag const& tag,
                boost::json::value& json_value,
                FeatureEventBase const& event) {
    auto& obj = json_value.emplace_object();
    obj.emplace("creationDate", boost::json::value_from(event.creation_date));
    obj.emplace("key", event.key);
    if (event.version) {
        obj.emplace("version", *event.version);
    }
    if (event.variation) {
        obj.emplace("variation", *event.variation);
    }
    obj.emplace("value", boost::json::value_from(event.value));
    if (event.reason) {
        obj.emplace("reason", boost::json::value_from(*event.reason));
    }
    obj.emplace("default", boost::json::value_from(event.default_));
}

void tag_invoke(boost::json::value_from_tag const& tag,
                boost::json::value& json_value,
                IdentifyEvent const& event) {
    auto& obj = json_value.emplace_object();
    obj.emplace("kind", "identify");
    obj.emplace("creationDate", boost::json::value_from(event.creation_date));
    obj.emplace("context", event.context);
}
}  // namespace launchdarkly::events::client

namespace launchdarkly::events {

void tag_invoke(boost::json::value_from_tag const& tag,
                boost::json::value& json_value,
                Date const& date) {
    json_value.emplace_int64() =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            date.t.time_since_epoch())
            .count();
}

void tag_invoke(boost::json::value_from_tag const& tag,
                boost::json::value& json_value,
                TrackEvent const& event) {
    auto& obj = json_value.emplace_object();
    obj.emplace("kind", "custom");
    obj.emplace("creationDate", boost::json::value_from(event.creation_date));
    obj.emplace("key", event.key);
    obj.emplace("contextKeys", boost::json::value_from(event.context_keys));
    if (event.data) {
        obj.emplace("data", boost::json::value_from(*event.data));
    }
    if (event.metric_value) {
        obj.emplace("metricValue", *event.metric_value);
    }
}

void tag_invoke(boost::json::value_from_tag const& tag,
                boost::json::value& json_value,
                events::OutputEvent const& event) {
    std::visit(
        [&](auto const& event) mutable { tag_invoke(tag, json_value, event); },
        event);
}

}  // namespace launchdarkly::events

namespace launchdarkly::events::detail {

void tag_invoke(boost::json::value_from_tag const& tag,
                boost::json::value& json_value,
                Summarizer::State const& state) {
    auto& obj = json_value.emplace_object();
    obj.emplace("default", boost::json::value_from(state.default_));
    obj.emplace("contextKinds", boost::json::value_from(state.context_kinds));
    boost::json::array counters;
    for (auto const& kvp : state.counters) {
        boost::json::object counter;
        if (kvp.first.version) {
            counter.emplace("version", *kvp.first.version);
        } else {
            counter.emplace("unknown", true);
        }
        if (kvp.first.variation) {
            counter.emplace("variation", *kvp.first.variation);
        }
        counter.emplace("value", boost::json::value_from(kvp.second.Value()));
        counter.emplace("count", kvp.second.Count());
        counters.push_back(std::move(counter));
    }
    obj.emplace("counters", std::move(counters));
}
void tag_invoke(boost::json::value_from_tag const& tag,
                boost::json::value& json_value,
                Summarizer const& summary) {
    auto& obj = json_value.emplace_object();
    obj.emplace("kind", "summary");
    obj.emplace("startDate",
                boost::json::value_from(Date{summary.start_time()}));
    obj.emplace("endDate", boost::json::value_from(Date{summary.end_time()}));
    obj.emplace("features", boost::json::value_from(summary.Features()));
}
}  // namespace launchdarkly::events::detail
