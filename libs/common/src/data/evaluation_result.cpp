#include <launchdarkly/data/evaluation_result.hpp>

#include <chrono>
#include <iomanip>
#include <utility>

namespace launchdarkly {

uint64_t EvaluationResult::Version() const {
    return version_;
}

std::optional<uint64_t> EvaluationResult::FlagVersion() const {
    return flag_version_;
}

bool EvaluationResult::TrackEvents() const {
    return track_events_;
}

bool EvaluationResult::TrackReason() const {
    return track_reason_;
}

std::optional<std::chrono::time_point<std::chrono::system_clock>>
EvaluationResult::DebugEventsUntilDate() const {
    return debug_events_until_date_;
}

EvaluationDetailInternal const& EvaluationResult::Detail() const {
    return detail_;
}

EvaluationResult::EvaluationResult(
    uint64_t version,
    std::optional<uint64_t> flag_version,
    bool track_events,
    bool track_reason,
    std::optional<std::chrono::time_point<std::chrono::system_clock>>
        debug_events_until_date,
    EvaluationDetailInternal detail)
    : version_(version),
      flag_version_(flag_version),
      track_events_(track_events),
      track_reason_(track_reason),
      debug_events_until_date_(debug_events_until_date),
      detail_(std::move(detail)) {}

std::ostream& operator<<(std::ostream& out, EvaluationResult const& result) {
    out << "{";
    out << " version: " << result.version_;
    out << " trackEvents: " << result.track_events_;
    out << " trackReason: " << result.track_reason_;

    if (result.debug_events_until_date_.has_value()) {
        std::time_t as_time_t = std::chrono::system_clock::to_time_t(
            result.debug_events_until_date_.value());
        out << " debugEventsUntilDate: "
            << std::put_time(std::gmtime(&as_time_t), "%Y-%m-%d %H:%M:%S");
    }
    out << " detail: " << result.detail_;
    out << "}";
    return out;
}

bool operator==(EvaluationResult const& lhs, EvaluationResult const& rhs) {
    return lhs.Version() == rhs.Version() &&
           lhs.TrackReason() == rhs.TrackReason() &&
           lhs.TrackEvents() == rhs.TrackEvents() &&
           lhs.Detail() == rhs.Detail() &&
           lhs.DebugEventsUntilDate() == rhs.DebugEventsUntilDate() &&
           lhs.FlagVersion() == rhs.FlagVersion();
}

bool operator!=(EvaluationResult const& lhs, EvaluationResult const& rhs) {
    return !(lhs == rhs);
}
}  // namespace launchdarkly
