#include "data/evaluation_result.hpp"
#include "value_mapping.hpp"

#include <chrono>

namespace launchdarkly {

uint64_t EvaluationResult::version() const {
    return version_;
}

std::optional<uint64_t> EvaluationResult::flag_version() const {
    return flag_version_;
}

bool EvaluationResult::track_events() const {
    return track_events_;
}

bool EvaluationResult::track_reason() const {
    return track_reason_;
}

std::optional<std::chrono::time_point<std::chrono::system_clock>>
EvaluationResult::debug_events_until_date() const {
    return debug_events_until_date_;
}

EvaluationDetail const& EvaluationResult::detail() const {
    return detail_;
}

EvaluationResult::EvaluationResult(
    uint64_t version,
    std::optional<uint64_t> flag_version,
    bool track_events,
    bool track_reason,
    std::optional<std::chrono::time_point<std::chrono::system_clock>>
        debug_events_until_date,
    EvaluationDetail detail)
    : version_(version),
      flag_version_(flag_version),
      track_events_(track_events),
      track_reason_(track_reason),
      debug_events_until_date_(debug_events_until_date),
      detail_(std::move(detail)) {}

EvaluationResult tag_invoke(
    boost::json::value_to_tag<EvaluationResult> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    if (json_value.is_object()) {
        auto json_obj = json_value.as_object();

        auto* version_iter = json_obj.find("version");
        auto version = ValueOrDefault(version_iter, json_obj.end(), 0UL);

        auto* flag_version_iter = json_obj.find("flagVersion");
        auto flag_version =
            ValueAsOpt<uint64_t>(flag_version_iter, json_obj.end());

        auto* track_events_iter = json_obj.find("trackEvents");
        auto track_events =
            ValueOrDefault(track_events_iter, json_obj.end(), false);

        auto* track_reason_iter = json_obj.find("trackReason");
        auto track_reason =
            ValueOrDefault(track_reason_iter, json_obj.end(), false);

        auto* debug_events_until_date_iter =
            json_obj.find("debugEventsUntilDate");

        auto debug_events_until_date =
            MapOpt<std::chrono::time_point<std::chrono::system_clock>,
                   uint64_t>(ValueAsOpt<uint64_t>(debug_events_until_date_iter,
                                                  json_obj.end()),
                             [](auto value) {
                                 return std::chrono::system_clock::time_point{
                                     std::chrono::milliseconds{value}};
                             });

        // Evaluation detail is directly de-serialized inline here.
        // This is because the shape of the evaluation detail is different
        // when deserializing FlagMeta. Primarily `variation` not
        // `variationIndex`.

        auto* value_iter = json_obj.find("value");
        auto value = value_iter != json_obj.end()
                         ? boost::json::value_to<Value>(value_iter->value())
                         : Value();

        auto* variation_iter = json_obj.find("variation");
        auto variation = ValueAsOpt<uint64_t>(variation_iter, json_obj.end());

        auto* reason_iter = json_obj.find("reason");
        auto reason =
            reason_iter != json_obj.end()
                ? std::make_optional(boost::json::value_to<EvaluationReason>(
                      reason_iter->value()))
                : std::nullopt;

        return {version,
                flag_version,
                track_events,
                track_reason,
                debug_events_until_date,
                EvaluationDetail(value, variation, reason)};
    }
    // This would represent malformed JSON.
    return {0,
            0,
            false,
            false,
            std::nullopt,
            EvaluationDetail(Value(), std::nullopt, std::nullopt)};
}
}  // namespace launchdarkly
