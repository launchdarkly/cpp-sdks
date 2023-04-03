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

}  // namespace launchdarkly
