#include "events/detail/summary_state.hpp"

namespace launchdarkly::events::detail {

SummaryState::SummaryState(std::chrono::system_clock::time_point start)
    : start_time_(start), counters_(), default_(), context_kinds_() {}

void SummaryState::update(InputEvent event) {}
}  // namespace launchdarkly::events::detail
