#include <launchdarkly/data/evaluation_detail.hpp>
#include <launchdarkly/value.hpp>
#include <string>

namespace launchdarkly {

template <typename T>
EvaluationDetail<T>::EvaluationDetail(
    T value,
    std::optional<std::size_t> variation_index,
    std::optional<launchdarkly::EvaluationReason> reason)
    : value_(std::move(value)),
      variation_index_(variation_index),
      reason_(std::move(reason)) {}

template <typename T>
EvaluationDetail<T>::EvaluationDetail(
    enum EvaluationReason::ErrorKind error_kind,
    T default_value)
    : value_(std::move(default_value)),
      variation_index_(std::nullopt),
      reason_(error_kind) {}

template <typename T>
EvaluationDetail<T>::EvaluationDetail(EvaluationReason reason)
    : value_(), variation_index_(std::nullopt), reason_(std::move(reason)) {}

template <typename T>
T const& EvaluationDetail<T>::Value() const {
    return value_;
}

template <typename T>
std::optional<EvaluationReason> const& EvaluationDetail<T>::Reason() const {
    return reason_;
}

template <typename T>
std::optional<std::size_t> EvaluationDetail<T>::VariationIndex() const {
    return variation_index_;
}
template <typename T>
T const& EvaluationDetail<T>::operator*() const {
    return value_;
}

template <typename T>
[[nodiscard]] bool EvaluationDetail<T>::IsError() const {
    return reason_.has_value() && reason_->ErrorKind().has_value();
}

template <typename T>
EvaluationDetail<T>::operator bool() const {
    return !IsError();
}

template class EvaluationDetail<bool>;
template class EvaluationDetail<int>;
template class EvaluationDetail<double>;
template class EvaluationDetail<std::string>;
template class EvaluationDetail<Value>;

}  // namespace launchdarkly
