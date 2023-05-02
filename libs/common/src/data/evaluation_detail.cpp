#include "data/evaluation_detail.hpp"

namespace launchdarkly {

EvaluationDetail::EvaluationDetail(launchdarkly::Value value,
                                   std::optional<std::size_t> variation_index,
                                   launchdarkly::EvaluationReason reason)
    : value_(std::move(value)),
      variation_index_(variation_index),
      reason_(std::move(reason)) {}

EvaluationDetail::EvaluationDetail(std::string error_kind,
                                   launchdarkly::Value default_value)
    : value_(std::move(default_value)),
      variation_index_(std::nullopt),
      reason_(error_kind) {}

Value const& EvaluationDetail::Value() const {
    return value_;
}

EvaluationReason const& EvaluationDetail::Reason() const {
    return reason_;
}

std::optional<std::size_t> EvaluationDetail::VariationIndex() const {
    return variation_index_;
}
}  // namespace launchdarkly
