#pragma once

#include <launchdarkly/server_side/client.hpp>
#include <launchdarkly/server_side/feature_flags_state.hpp>

#include "../data_store/data_store.hpp"
#include "../evaluation/evaluator.hpp"

namespace launchdarkly::server_side {
class AllFlagsStateBuilder {
   public:
    AllFlagsStateBuilder(evaluation::Evaluator& evaluator,
                         data_store::IDataStore const& store);

    [[nodiscard]] FeatureFlagsState Build(Context const& context,
                                          enum AllFlagsStateOptions options);

   private:
    evaluation::Evaluator& evaluator_;
    data_store::IDataStore const& store_;
};
}  // namespace launchdarkly::server_side
