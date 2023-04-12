#include "serialization/json_evaluation_reason.hpp"
#include "serialization/value_mapping.hpp"

namespace launchdarkly {
tl::expected<EvaluationReason, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<EvaluationReason, JsonError>> const&
        unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    if (json_value.is_object()) {
        auto& json_obj = json_value.as_object();

        auto kind_iter = json_obj.find("kind");
        auto kind = ValueAsOpt<std::string>(kind_iter, json_obj.end());
        if (!kind.has_value()) {
            return tl::unexpected(JsonError::kSchemaFailure);
        }

        auto* error_kind_iter = json_obj.find("errorKind");
        auto error_kind =
            ValueAsOpt<std::string>(error_kind_iter, json_obj.end());

        auto* rule_index_iter = json_obj.find("ruleIndex");
        auto rule_index = ValueAsOpt<uint64_t>(rule_index_iter, json_obj.end());

        auto* rule_id_iter = json_obj.find("ruleId");
        auto rule_id = ValueAsOpt<std::string>(rule_id_iter, json_obj.end());

        auto* prerequisite_key_iter = json_obj.find("prerequisiteKey");
        auto prerequisite_key =
            ValueAsOpt<std::string>(prerequisite_key_iter, json_obj.end());

        auto* in_experiment_iter = json_obj.find("inExperiment");
        auto in_experiment =
            ValueOrDefault(in_experiment_iter, json_obj.end(), false);

        auto* big_segment_status_iter = json_obj.find("bigSegmentStatus");
        auto big_segment_status =
            ValueAsOpt<std::string>(big_segment_status_iter, json_obj.end());

        return EvaluationReason{std::string(kind.value()),
                                error_kind,
                                rule_index,
                                rule_id,
                                prerequisite_key,
                                in_experiment,
                                big_segment_status};
    }
    return tl::unexpected(JsonError::kSchemaFailure);
}

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                EvaluationReason const& reason) {
    auto& obj = json_value.emplace_object();
    obj.emplace("kind", reason.kind());
    if (auto error_kind = reason.error_kind()) {
        obj.emplace("errorKind", *error_kind);
    }
    if (auto big_segment_status = reason.big_segment_status()) {
        obj.emplace("bigSegmentStatus", *big_segment_status);
    }
    if (auto rule_id = reason.rule_id()) {
        obj.emplace("ruleId", *rule_id);
    }
    if (auto rule_index = reason.rule_index()) {
        obj.emplace("ruleIndex", *rule_index);
    }
    if (reason.in_experiment()) {
        obj.emplace("inExperiment", true);
    }
    if (auto prereq_key = reason.prerequisite_key()) {
        obj.emplace("prerequisiteKey", *prereq_key);
    }
}
}  // namespace launchdarkly
