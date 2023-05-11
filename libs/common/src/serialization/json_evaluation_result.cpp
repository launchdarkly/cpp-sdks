#include "launchdarkly/serialization/json_evaluation_result.hpp"
#include "launchdarkly/serialization/json_evaluation_reason.hpp"
#include "launchdarkly/serialization/json_value.hpp"
#include "launchdarkly/serialization/value_mapping.hpp"

namespace launchdarkly {
tl::expected<EvaluationResult, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<EvaluationResult, JsonError>> const&
        unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    if (json_value.is_object()) {
        auto& json_obj = json_value.as_object();

        auto* version_iter = json_obj.find("version");
        auto version_opt = ValueAsOpt<uint64_t>(version_iter, json_obj.end());
        if (!version_opt.has_value()) {
            return tl::unexpected(JsonError::kSchemaFailure);
        }
        auto version = version_opt.value();

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
        if (value_iter == json_obj.end()) {
            return tl::unexpected(JsonError::kSchemaFailure);
        }
        auto value = boost::json::value_to<Value>(value_iter->value());

        auto* variation_iter = json_obj.find("variation");
        auto variation = ValueAsOpt<uint64_t>(variation_iter, json_obj.end());

        auto* reason_iter = json_obj.find("reason");

        // There is a reason.
        if (reason_iter != json_obj.end()) {
            auto reason = boost::json::value_to<
                tl::expected<EvaluationReason, JsonError>>(
                reason_iter->value());

            if (reason.has_value()) {
                return EvaluationResult{
                    version,
                    flag_version,
                    track_events,
                    track_reason,
                    debug_events_until_date,
                    EvaluationDetailInternal(
                        value, variation, std::make_optional(reason.value()))};
            }
            // We could not parse the reason.
            return tl::unexpected(JsonError::kSchemaFailure);
        }

        // There was no reason.
        return EvaluationResult{
            version,
            flag_version,
            track_events,
            track_reason,
            debug_events_until_date,
            EvaluationDetailInternal(value, variation, std::nullopt)};
    }

    return tl::unexpected(JsonError::kSchemaFailure);
}
}  // namespace launchdarkly
