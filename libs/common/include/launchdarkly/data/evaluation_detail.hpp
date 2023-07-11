#pragma once

#include <cstddef>
#include <optional>

#include <launchdarkly/data/evaluation_reason.hpp>

namespace launchdarkly {

/**
 * EvaluationDetail contains additional metadata related to a feature flag
 * evaluation. To obtain an instance of EvaluationDetail, use a variation method
 * suffixed with Detail, such as BoolVariationDetail.
 * @tparam T The primitive variation value, which is limited to
 *  bool, int, double, std::string, and launchdarkly::Value.
 */
template <typename T>
class EvaluationDetail {
   public:
    /**
     * Constructs an EvaluationDetail from results of an evaluation.
     * @param value The variation value.
     * @param variation_index The variation index.
     * @param reason The reason for the results.
     */
    EvaluationDetail(T value,
                     std::optional<std::size_t> variation_index,
                     std::optional<EvaluationReason> reason);

    /**
     * Constructs an EvaluationDetail representing an error and a default
     * value.
     * @param error_kind Kind of the error.
     * @param default_value Default value.
     */
    EvaluationDetail(enum EvaluationReason::ErrorKind error_kind,
                     T default_value);

    /**
     * Constructs an EvaluationDetail consisting of a reason but no value.
     * This is used when a flag has no appropriate fallback value.
     * @param reason The reason.
     */
    EvaluationDetail(EvaluationReason reason);

    /**
     * @return A reference to the variation value. For convenience, the *
     * operator may also be used to obtain the value.
     */
    [[nodiscard]] T const& Value() const;

    /**
     * @return A variation index, if this was a successful evaluation;
     * otherwise, std::nullopt.
     */
    [[nodiscard]] std::optional<std::size_t> VariationIndex() const;

    /**
     * @return A reference to the reason for the results.
     */
    [[nodiscard]] std::optional<EvaluationReason> const& Reason() const;

    /**
     * @return A reference to the variation value.
     */
    T const& operator*() const;

   private:
    T value_;
    std::optional<std::size_t> variation_index_;
    std::optional<EvaluationReason> reason_;
};

/*
 * Holds details for the C bindings, omitting the generic type parameter that is
 * needed for EvaluationDetail<T>. Instead, the bindings will directly return
 * the evaluation result, and fill in a detail structure using an out parameter.
 */
struct CEvaluationDetail {
    template <typename T>
    CEvaluationDetail(EvaluationDetail<T> const& detail)
        : variation_index(detail.VariationIndex()), reason(detail.Reason()) {}
    std::optional<std::size_t> variation_index;
    std::optional<EvaluationReason> reason;
};

}  // namespace launchdarkly
