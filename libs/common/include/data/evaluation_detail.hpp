#pragma once

#include <cstddef>
#include <optional>
#include <ostream>
#include <utility>

#include "data/evaluation_reason.hpp"
#include "value.hpp"

namespace launchdarkly {

class EvaluationDetail {
   public:
    EvaluationDetail(Value value,
                     std::optional<std::size_t> variation_index,
                     EvaluationReason reason);

    EvaluationDetail(std::string error_kind, Value default_value);

    [[nodiscard]] class launchdarkly::Value const& Value() const;
    [[nodiscard]] std::optional<std::size_t> VariationIndex() const;
    [[nodiscard]] EvaluationReason const& Reason() const;

    class launchdarkly::Value const& operator*() const;

   private:
    class launchdarkly::Value value_;
    std::optional<std::size_t> variation_index_;
    EvaluationReason reason_;
};
}  // namespace launchdarkly
