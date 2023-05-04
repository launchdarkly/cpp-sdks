#pragma once

#include <cstddef>
#include <optional>

#include "data/evaluation_reason.hpp"

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
                     EvaluationReason reason);

    /**
     * Constructs an EvaluationDetail representing an error and a default
     * value.
     * @param error_kind Kind of the error.
     * @param default_value Default value.
     */
    EvaluationDetail(std::string error_kind, T default_value);

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
    [[nodiscard]] EvaluationReason const& Reason() const;

    /**
     * @return A reference to the variation value.
     */
    T const& operator*() const;

   private:
    T value_;
    std::optional<std::size_t> variation_index_;
    EvaluationReason reason_;
};
}  // namespace launchdarkly
