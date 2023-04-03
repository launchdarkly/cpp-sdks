#include "data/evaluation_detail.hpp"

namespace launchdarkly {

Value const& EvaluationDetail::value() const {
    return value_;
}

std::optional<std::size_t> EvaluationDetail::variation_index() const {
    return variation_index_;
}

std::optional<std::reference_wrapper<EvaluationReason const>>
EvaluationDetail::reason() const {
    return reason_;
}
EvaluationDetail::EvaluationDetail(Value value,
                                   std::optional<std::size_t> variation_index,
                                   std::optional<EvaluationReason> reason)
    : value_(std::move(value)),
      variation_index_(variation_index),
      reason_(std::move(reason)) {}

}  // namespace launchdarkly
