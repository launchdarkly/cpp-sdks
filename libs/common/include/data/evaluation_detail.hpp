#pragma once

#include <cstddef>

#include "value.hpp"
#include "data/evaluation_reason.hpp"

namespace launchdarkly {

class EvaluationDetail {
    Value const& value() const;
    std::size_t variation_index() const;
    EvaluationReason const& reason() const;
};

}  // namespace launchdarkly
