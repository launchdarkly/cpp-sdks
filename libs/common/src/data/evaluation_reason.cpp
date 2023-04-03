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
}  // namespace launchdarkly
