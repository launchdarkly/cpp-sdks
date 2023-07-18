#pragma once
#include <launchdarkly/data_model/flag.hpp>

namespace launchdarkly::server_side::evaluation::operators {

bool Match(data_model::Clause::Op op,
           Value const& context_value,
           Value const& clause_value);

}  // namespace launchdarkly::server_side::evaluation::operators
