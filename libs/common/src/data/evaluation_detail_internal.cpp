#include "data/evaluation_detail_internal.hpp"

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

}  // namespace launchdarkly
