#include "data/evaluation_reason.hpp"

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

std::ostream& operator<<(std::ostream& out, EvaluationReason const& reason) {
    out << "{";
    out << " kind: " << reason.kind_;
    if (reason.error_kind_.has_value()) {
        out << " errorKind: " << reason.error_kind_.value();
    }
    if (reason.rule_index_.has_value()) {
        out << " ruleIndex: " << reason.rule_index_.value();
    }
    if (reason.rule_id()) {
        out << " ruleId: " << reason.rule_id_.value();
    }
    if (reason.prerequisite_key_.has_value()) {
        out << " prerequisiteKey: " << reason.prerequisite_key_.value();
    }
    out << " inExperiment: " << reason.in_experiment_;
    if (reason.big_segment_status_.has_value()) {
        out << " bigSegmentStatus: " << reason.big_segment_status_.value();
    }
    out << "}";
    return out;
}

bool operator==(EvaluationReason const& lhs, EvaluationReason const& rhs) {
    return lhs.kind() == rhs.kind() && lhs.error_kind() == rhs.error_kind() &&
           lhs.in_experiment() == rhs.in_experiment() &&
           lhs.big_segment_status() == rhs.big_segment_status() &&
           lhs.prerequisite_key() == rhs.prerequisite_key() &&
           lhs.rule_id() == rhs.rule_id() &&
           lhs.rule_index() == rhs.rule_index();
}

bool operator!=(EvaluationReason const& lhs, EvaluationReason const& rhs) {
    return !(lhs == rhs);
}
}  // namespace launchdarkly
