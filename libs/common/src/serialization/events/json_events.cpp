#include "serialization/events/json_events.hpp"
#include "serialization/json_context.hpp"
#include "serialization/json_evaluation_reason.hpp"
#include "serialization/json_value.hpp"

namespace launchdarkly::events {

void tag_invoke(boost::json::value_from_tag const& tag,
                boost::json::value& json_value,
                FeatureEvent const& event) {
    tag_invoke(tag, json_value, event.base);
    json_value.as_object().emplace("kind", "feature");
    json_value.as_object().emplace("contextKeys",
                                   boost::json::value_from(event.context_keys));
}

void tag_invoke(boost::json::value_from_tag const& tag,
                boost::json::value& json_value,
                DebugEvent const& event) {
    tag_invoke(tag, json_value, event.base);
    json_value.as_object().emplace("kind", "debug");
    json_value.as_object().emplace("context",
                                   boost::json::value_from(event.context));
}

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                FeatureEventFields const& event) {
    auto& obj = json_value.emplace_object();
    obj.emplace("creationDate",
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    event.creation_date.time_since_epoch())
                    .count());
    obj.emplace("key", event.key);
    obj.emplace("version", event.version);
    if (event.variation) {
        obj.emplace("variation", *event.variation);
    }
    obj.emplace("value", boost::json::value_from(event.value));
    if (event.reason) {
        obj.emplace("reason", boost::json::value_from(*event.reason));
    }
    obj.emplace("default", boost::json::value_from(event.default_));
    if (event.prereq_of) {
        obj.emplace("prereqOf", *event.prereq_of);
    }
}

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                IdentifyEvent const& event) {
    auto& obj = json_value.emplace_object();
    obj.emplace("kind", "identify");
    obj.emplace("creationDate",
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    event.creation_date.time_since_epoch())
                    .count());
    obj.emplace("context", boost::json::value_from(event.context));
}
// void tag_invoke(boost::json::value_from_tag const& tag,
//                 boost::json::value& json_value,
//                 events::OutputEvent const& event) {
//     std::visit([&](auto const& e) mutable { tag_invoke(tag, json_value, e);
//     })
// }
}  // namespace launchdarkly::events
