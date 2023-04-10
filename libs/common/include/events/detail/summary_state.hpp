#pragma once
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include "events/events.hpp"
#include "value.hpp"

namespace launchdarkly::events::detail {

class SummaryState {
   public:
    SummaryState(std::chrono::system_clock::time_point start);
    void update(InputEvent event);

   private:
    std::chrono::system_clock::time_point start_time_;
    std::unordered_map<VariationKey, VariationSummary, VariationKey::Hash>
        counters_;
    Value default_;
    std::unordered_set<std::string> context_kinds_;
};

}  // namespace launchdarkly::events::detail
