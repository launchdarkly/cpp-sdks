#include "data/evaluation_result.hpp"
#include "serialization/value_mapping.hpp"

#include <chrono>

namespace launchdarkly {

uint64_t EvaluationResult::version() const {
    return version_;
}

std::optional<uint64_t> EvaluationResult::flag_version() const {
    return flag_version_;
}

bool EvaluationResult::track_events() const {
    return track_events_;
}

bool EvaluationResult::track_reason() const {
    return track_reason_;
}

std::optional<std::chrono::time_point<std::chrono::system_clock>>
EvaluationResult::debug_events_until_date() const {
    return debug_events_until_date_;
}

EvaluationDetail const& EvaluationResult::detail() const {
    return detail_;
}

EvaluationResult::EvaluationResult(
    uint64_t version,
    std::optional<uint64_t> flag_version,
    bool track_events,
    bool track_reason,
    std::optional<std::chrono::time_point<std::chrono::system_clock>>
        debug_events_until_date,
    EvaluationDetail detail)
    : version_(version),
      flag_version_(flag_version),
      track_events_(track_events),
      track_reason_(track_reason),
      debug_events_until_date_(debug_events_until_date),
      detail_(std::move(detail)) {}

}  // namespace launchdarkly
