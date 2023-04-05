#include "data/evaluation_reason.hpp"
#include "serialization/value_mapping.hpp"

namespace launchdarkly {

std::string const& EvaluationReason::kind() const {
    return kind_;
}

std::optional<std::string> EvaluationReason::error_kind() const {
    return error_kind_;
}

std::optional<std::size_t> EvaluationReason::rule_index() const {
    return rule_index_;
}

std::optional<std::string> EvaluationReason::rule_id() const {
    return rule_id_;
}

std::optional<std::string> EvaluationReason::prerequisite_key() const {
    return prerequisite_key_;
}

bool EvaluationReason::in_experiment() const {
    return in_experiment_;
}

std::optional<std::string> EvaluationReason::big_segment_status() const {
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
      rule_index_(rule_index),
      rule_id_(std::move(rule_id)),
      prerequisite_key_(std::move(prerequisite_key)),
      in_experiment_(in_experiment),
      big_segment_status_(std::move(big_segment_status)) {}

}  // namespace launchdarkly
