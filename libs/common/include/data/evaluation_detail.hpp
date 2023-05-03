#pragma once

#include <cstddef>
#include <optional>

#include "data/evaluation_reason.hpp"

namespace launchdarkly {

template <typename T>
class EvaluationDetail {
   public:
    EvaluationDetail(T value,
                     std::optional<std::size_t> variation_index,
                     EvaluationReason reason);

    EvaluationDetail(std::string error_kind, T default_value);

    [[nodiscard]] T const& Value() const;
    [[nodiscard]] std::optional<std::size_t> VariationIndex() const;
    [[nodiscard]] EvaluationReason const& Reason() const;

    T const& operator*() const;

   private:
    T value_;
    std::optional<std::size_t> variation_index_;
    EvaluationReason reason_;
};
}  // namespace launchdarkly
