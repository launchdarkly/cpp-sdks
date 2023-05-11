#include "launchdarkly/data/evaluation_detail_internal.hpp"

namespace launchdarkly {

Value const& EvaluationDetailInternal::value() const {
    return value_;
}

std::optional<std::size_t> EvaluationDetailInternal::variation_index() const {
    return variation_index_;
}

std::optional<std::reference_wrapper<EvaluationReason const>>
EvaluationDetailInternal::reason() const {
    return reason_;
}
EvaluationDetailInternal::EvaluationDetailInternal(
    Value value,
    std::optional<std::size_t> variation_index,
    std::optional<EvaluationReason> reason)
    : value_(std::move(value)),
      variation_index_(variation_index),
      reason_(std::move(reason)) {}

std::ostream& operator<<(std::ostream& out,
                         EvaluationDetailInternal const& detail) {
    out << "{";
    out << " value: " << detail.value_;
    if (detail.variation_index_.has_value()) {
        out << " variationIndex: " << detail.variation_index_.value();
    }
    if (detail.reason_.has_value()) {
        out << " reason: " << detail.reason_.value();
    }
    out << "}";
    return out;
}

bool operator==(EvaluationDetailInternal const& lhs,
                EvaluationDetailInternal const& rhs) {
    return lhs.value() == rhs.value() && lhs.reason() == rhs.reason() &&
           lhs.variation_index() == rhs.variation_index();
}

bool operator!=(EvaluationDetailInternal const& lhs,
                EvaluationDetailInternal const& rhs) {
    return !(lhs == rhs);
}

}  // namespace launchdarkly
