#include "serialization/json_evaluation_reason.hpp"
#include "serialization/value_mapping.hpp"

#include <sstream>

namespace launchdarkly {

tl::expected<EvaluationReason::Kind, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<EvaluationReason::Kind, JsonError>> const& unused,
    boost::json::value const& json_value) {
    if (!json_value.is_string()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    auto const& str = json_value.as_string();
    if (str == "OFF") {
        return EvaluationReason::Kind::kOff;
    }
    if (str == "FALLTHROUGH") {
        return EvaluationReason::Kind::kFallthrough;
    }
    if (str == "TARGET_MATCH") {
        return EvaluationReason::Kind::kTargetMatch;
    }
    if (str == "RULE_MATCH") {
        return EvaluationReason::Kind::kRuleMatch;
    }
    if (str == "PREREQUISITE_FAILED") {
        return EvaluationReason::Kind::kPrerequisiteFailed;
    }
    if (str == "ERROR") {
        return EvaluationReason::Kind::kError;
    }
    return tl::make_unexpected(JsonError::kSchemaFailure);
}

tl::expected<EvaluationReason, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<EvaluationReason, JsonError>> const&
        unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    if (json_value.is_object()) {
        auto& json_obj = json_value.as_object();

        auto kind_iter = json_obj.find("kind");
        if (kind_iter == json_obj.end()) {
            return tl::make_unexpected(JsonError::kSchemaFailure);
        }

        auto kind = boost::json::value_to<
            tl::expected<EvaluationReason::Kind, JsonError>>(
            kind_iter->value());

        if (!kind) {
            return tl::make_unexpected(kind.error());
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

        return EvaluationReason{*kind,
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
                EvaluationReason::Kind const& kind) {
    auto& str = json_value.emplace_string();
    std::ostringstream oss;
    oss << kind;
    str = oss.str();
}
void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                EvaluationReason const& reason) {
    auto& obj = json_value.emplace_object();
    obj.emplace("kind", boost::json::value_from(reason.kind()));
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
