#include "serialization/json_evaluation_reason.hpp"
#include "serialization/value_mapping.hpp"

namespace launchdarkly {
EvaluationReason tag_invoke(
    boost::json::value_to_tag<EvaluationReason> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    if (json_value.is_object()) {
        auto json_obj = json_value.as_object();

        auto* kind_iter = json_obj.find("kind");
        auto kind =
            ValueOrDefault<std::string>(kind_iter, json_obj.end(), "ERROR");

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

        return {std::string(kind), error_kind,    rule_index,        rule_id,
                prerequisite_key,  in_experiment, big_segment_status};
    }
    return {"ERROR",      std::nullopt, 0,           std::nullopt,
            std::nullopt, false,        std::nullopt};
}
}  // namespace launchdarkly
