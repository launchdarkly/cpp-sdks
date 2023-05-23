#include <launchdarkly/data/evaluation_reason.hpp>

namespace launchdarkly {

enum EvaluationReason::Kind const& EvaluationReason::Kind() const {
    return kind_;
}

std::optional<enum EvaluationReason::ErrorKind> EvaluationReason::ErrorKind()
    const {
    return error_kind_;
}

std::optional<std::size_t> EvaluationReason::RuleIndex() const {
    return rule_index_;
}

std::optional<std::string> EvaluationReason::RuleId() const {
    return rule_id_;
}

std::optional<std::string> EvaluationReason::PrerequisiteKey() const {
    return prerequisite_key_;
}

bool EvaluationReason::InExperiment() const {
    return in_experiment_;
}

std::optional<std::string> EvaluationReason::BigSegmentStatus() const {
    return big_segment_status_;
}

EvaluationReason::EvaluationReason(
    enum Kind kind,
    std::optional<enum ErrorKind> error_kind,
    std::optional<std::size_t> rule_index,
    std::optional<std::string> rule_id,
    std::optional<std::string> prerequisite_key,
    bool in_experiment,
    std::optional<std::string> big_segment_status)
    : kind_(kind),
      error_kind_(error_kind),
      rule_index_(rule_index),
      rule_id_(std::move(rule_id)),
      prerequisite_key_(std::move(prerequisite_key)),
      in_experiment_(in_experiment),
      big_segment_status_(std::move(big_segment_status)) {}

EvaluationReason::EvaluationReason(enum ErrorKind error_kind)
    : EvaluationReason(Kind::kError,
                       error_kind,
                       std::nullopt,
                       std::nullopt,
                       std::nullopt,
                       false,
                       std::nullopt) {}

std::ostream& operator<<(std::ostream& out, EvaluationReason const& reason) {
    out << "{";
    out << " kind: " << reason.kind_;
    if (reason.error_kind_.has_value()) {
        out << " errorKind: " << reason.error_kind_.value();
    }
    if (reason.rule_index_.has_value()) {
        out << " ruleIndex: " << reason.rule_index_.value();
    }
    if (reason.RuleId()) {
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
    return lhs.Kind() == rhs.Kind() && lhs.ErrorKind() == rhs.ErrorKind() &&
           lhs.InExperiment() == rhs.InExperiment() &&
           lhs.BigSegmentStatus() == rhs.BigSegmentStatus() &&
           lhs.PrerequisiteKey() == rhs.PrerequisiteKey() &&
           lhs.RuleId() == rhs.RuleId() && lhs.RuleIndex() == rhs.RuleIndex();
}

bool operator!=(EvaluationReason const& lhs, EvaluationReason const& rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& out,
                         enum EvaluationReason::Kind const& kind) {
    switch (kind) {
        case EvaluationReason::Kind::kOff:
            out << "OFF";
            break;
        case EvaluationReason::Kind::kFallthrough:
            out << "FALLTHROUGH";
            break;
        case EvaluationReason::Kind::kTargetMatch:
            out << "TARGET_MATCH";
            break;
        case EvaluationReason::Kind::kRuleMatch:
            out << "RULE_MATCH";
            break;
        case EvaluationReason::Kind::kPrerequisiteFailed:
            out << "PREREQUISITE_FAILED";
            break;
        case EvaluationReason::Kind::kError:
            out << "ERROR";
            break;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out,
                         enum EvaluationReason::ErrorKind const& kind) {
    switch (kind) {
        case EvaluationReason::ErrorKind::kClientNotReady:
            out << "CLIENT_NOT_READY";
            break;
        case EvaluationReason::ErrorKind::kUserNotSpecified:
            out << "USER_NOT_SPECIFIED";
            break;
        case EvaluationReason::ErrorKind::kFlagNotFound:
            out << "FLAG_NOT_FOUND";
            break;
        case EvaluationReason::ErrorKind::kWrongType:
            out << "WRONG_TYPE";
            break;
        case EvaluationReason::ErrorKind::kMalformedFlag:
            out << "MALFORMED_FLAG";
            break;
        case EvaluationReason::ErrorKind::kException:
            out << "EXCEPTION";
            break;
    }
    return out;
}

}  // namespace launchdarkly
