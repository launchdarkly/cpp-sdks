#pragma once

#include <cstddef>
#include <optional>
#include <refwrap>

#include "value.hpp"
#include "data/evaluation_reason.hpp"

namespace launchdarkly {

/**
 * An object that combines the result of a feature flag evaluation with information about
 * how it was calculated.
 *
 * This is the result of calling [TODO: Evaluate detail].
 *
 * For more information, see the [SDK reference guide]
 * (https://docs.launchdarkly.com/sdk/features/evaluation-reasons#TODO).
 */
class EvaluationDetail {
   public:
    /**
    * The result of the flag evaluation. This will be either one of the flag's variations or
    * the default value that was passed to [TODO: Evaluate detail].
    */
    Value const& value() const;

    /**
    * The index of the returned value within the flag's list of variations, e.g. 0 for the
    * first variation-- or `nullopt` if the default value was returned.
    */
    std::optional<std::size_t> variation_index() const;

    /**
    * An object describing the main factor that influenced the flag evaluation value.
    */
    [[nodiscard]] std::optional<std::reference_wrapper<const EvaluationReason>> reason() const;

   private:
    Value value_;
    std::optional<std::size_t> variation_index_;
    std::optional<EvaluationReason> reason_;
};

}  // namespace launchdarkly
