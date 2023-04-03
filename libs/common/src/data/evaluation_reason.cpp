#include "data/evaluation_reason.hpp"

namespace launchdarkly {

std::string const& EvaluationReason::kind() const {
    return kind_;
}

std::optional<std::string_view> EvaluationReason::error_kind() const {
    return error_kind_;
}

std::optional<std::size_t> EvaluationReason::rule_index() const {
    return rule_index_;
}

std::optional<std::string_view> EvaluationReason::rule_id() const {
    return rule_id_;
}

std::optional<std::string_view> EvaluationReason::prerequisite_key() const {
    return prerequisite_key_;
}

bool EvaluationReason::in_experiment() const {
    return in_experiment_;
}

std::optional<std::string_view> EvaluationReason::big_segment_status() const {
    return big_segment_status_;
}

EvaluationReason::EvaluationReason(
    std::string kind,
    std::optional<std::string> error_kind,
    std::optional<std::size_t> rule_index,
    std::optional<std::string> rule_id,
    std::optional<std::string> prerequisite_key,
    bool in_experiment,
    std::optional<std::string> big_segment_status)
    : kind_(std::move(kind)),
      error_kind_(std::move(error_kind)),
      rule_index_(std::move(rule_index)),
      rule_id_(std::move(rule_id)),
      prerequisite_key_(std::move(prerequisite_key)),
      in_experiment_(in_experiment),
      big_segment_status_(std::move(big_segment_status)) {}

static std::optional<std::string> IterToOptionalString(
    boost::json::key_value_pair* iter,
    boost::json::key_value_pair* end) {
    return iter != end && iter->value().is_string()
               ? std::make_optional(std::string(iter->value().as_string()))
               : std::nullopt;
}

EvaluationReason tag_invoke(
    boost::json::value_to_tag<EvaluationReason> const& unused,
    boost::json::value const& json_value) {
    if (json_value.is_object()) {
        auto json_obj = json_value.as_object();

        auto kind_iter = json_obj.find("kind");
        auto kind =
            kind_iter != json_obj.end() && kind_iter->value().is_string()
                ? kind_iter->value().as_string()
                : "ERROR";

        auto error_kind_iter = json_obj.find("errorKind");
        auto error_kind = IterToOptionalString(error_kind_iter, json_obj.end());

        auto rule_index_iter = json_obj.find("ruleIndex");
        auto rule_index =
            rule_index_iter != json_obj.end() &&
                    rule_index_iter->value().is_number()
                ? std::make_optional(
                      rule_index_iter->value().to_number<uint64_t>())
                : std::nullopt;

        auto rule_id_iter = json_obj.find("ruleId");
        auto rule_id = IterToOptionalString(rule_id_iter, json_obj.end());

        auto prerequisite_key_iter = json_obj.find("prerequisiteKey");
        auto prerequisite_key =
            IterToOptionalString(prerequisite_key_iter, json_obj.end());

        auto in_experiment_iter = json_obj.find("inExperiment");
        auto in_experiment = in_experiment_iter != json_obj.end() &&
                             in_experiment_iter->value().is_bool() &&
                             in_experiment_iter->value().as_bool();

        auto big_segment_status_iter = json_obj.find("bigSegmentStatus");
        auto big_segment_status =
            IterToOptionalString(big_segment_status_iter, json_obj.end());

        return {std::string(kind), error_kind,    rule_index,        rule_id,
                prerequisite_key,  in_experiment, big_segment_status};
    }
    return {"ERROR",      std::nullopt, 0,           std::nullopt,
            std::nullopt, false,        std::nullopt};
}
}  // namespace launchdarkly
