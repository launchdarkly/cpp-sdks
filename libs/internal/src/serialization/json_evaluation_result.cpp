#include <launchdarkly/detail/serialization/json_errors.hpp>
#include <launchdarkly/detail/serialization/json_value.hpp>
#include <launchdarkly/serialization/json_evaluation_reason.hpp>
#include <launchdarkly/serialization/json_evaluation_result.hpp>
#include <launchdarkly/serialization/value_mapping.hpp>

#include <boost/core/ignore_unused.hpp>
#include <boost/json.hpp>

namespace launchdarkly {
tl::expected<std::optional<EvaluationResult>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<EvaluationResult>, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    if (json_value.is_null()) {
        return std::nullopt;
    }
    if (!json_value.is_object()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    auto const& json_obj = json_value.as_object();

    auto* version_iter = json_obj.find("version");
    auto version_opt = ValueAsOpt<uint64_t>(version_iter, json_obj.end());
    if (!version_opt.has_value()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    auto version = version_opt.value();

    auto* flag_version_iter = json_obj.find("flagVersion");
    auto flag_version = ValueAsOpt<uint64_t>(flag_version_iter, json_obj.end());

    auto* track_events_iter = json_obj.find("trackEvents");
    auto track_events =
        ValueOrDefault(track_events_iter, json_obj.end(), false);

    auto* track_reason_iter = json_obj.find("trackReason");
    auto track_reason =
        ValueOrDefault(track_reason_iter, json_obj.end(), false);

    auto* debug_events_until_date_iter = json_obj.find("debugEventsUntilDate");

    auto debug_events_until_date =
        MapOpt<std::chrono::time_point<std::chrono::system_clock>, uint64_t>(
            ValueAsOpt<uint64_t>(debug_events_until_date_iter, json_obj.end()),
            [](auto value) {
                return std::chrono::system_clock::time_point{
                    std::chrono::milliseconds{value}};
            });

    auto* prerequisites_iter = json_obj.find("prerequisites");
    auto prerequisites = ValueAsOpt<std::vector<std::string>>(
        prerequisites_iter, json_obj.end());

    // Evaluation detail is directly de-serialized inline here.
    // This is because the shape of the evaluation detail is different
    // when deserializing FlagMeta. Primarily `variation` not
    // `variationIndex`.

    // todo(cwaldren): SC-203949 + SC-236165
    // We're looking for the evaluated value. If it's not there, we should treat
    // it as null and *not* a schema error. This is because the server-side SDK
    // that produced the EvaluationResult may have omitted the 'null' value.
    // Otherwise, if it is there, it must deserialize to a valid Value (which
    // might itself be null.)
    Value value = Value::Null();
    if (auto* value_iter = json_obj.find("value");
        value_iter != json_obj.end()) {
        auto maybe_value =
            boost::json::value_to<tl::expected<Value, JsonError>>(
                value_iter->value());
        if (!maybe_value) {
            return tl::unexpected(maybe_value.error());
        }
        value = *maybe_value;
    }

    auto* variation_iter = json_obj.find("variation");
    auto variation = ValueAsOpt<uint64_t>(variation_iter, json_obj.end());

    auto* reason_iter = json_obj.find("reason");

    // There is a reason.
    if (reason_iter != json_obj.end() && !reason_iter->value().is_null()) {
        auto reason =
            boost::json::value_to<tl::expected<EvaluationReason, JsonError>>(
                reason_iter->value());

        if (reason.has_value()) {
            return EvaluationResult{
                version,
                flag_version,
                track_events,
                track_reason,
                debug_events_until_date,
                EvaluationDetailInternal(std::move(value), variation,
                                         std::make_optional(reason.value()))};
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
        EvaluationDetailInternal(std::move(value), variation, std::nullopt),
        prerequisites};
}

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                EvaluationResult const& evaluation_result) {
    boost::ignore_unused(unused);

    auto& obj = json_value.emplace_object();
    obj.emplace("version", evaluation_result.Version());

    if (evaluation_result.FlagVersion()) {
        obj.emplace("flagVersion", *evaluation_result.FlagVersion());
    }

    if (evaluation_result.TrackEvents()) {
        obj.emplace("trackEvents", evaluation_result.TrackEvents());
    }

    if (evaluation_result.TrackReason()) {
        obj.emplace("trackReason", evaluation_result.TrackReason());
    }

    if (evaluation_result.DebugEventsUntilDate()) {
        obj.emplace(
            "debugEventsUntilDate",
            std::chrono::duration_cast<std::chrono::milliseconds>(
                evaluation_result.DebugEventsUntilDate()->time_since_epoch())
                .count());
    }

    if (auto const prerequisites = evaluation_result.Prerequisites()) {
        if (!prerequisites->empty()) {
            obj.emplace("prerequisites",
                        boost::json::value_from(prerequisites.value()));
        }
    }

    auto& detail = evaluation_result.Detail();
    auto value_json = boost::json::value_from(detail.Value());
    obj.emplace("value", value_json);

    if (detail.VariationIndex()) {
        obj.emplace("variationIndex", *detail.VariationIndex());
    }

    if (detail.Reason()) {
        auto reason_json = boost::json::value_from(*detail.Reason());
        obj.emplace("reason", reason_json);
    }
}
}  // namespace launchdarkly
