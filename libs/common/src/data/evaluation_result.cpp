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

std::optional<std::vector<std::string>> const& EvaluationResult::Prerequisites()
    const {
    return prerequisites_;
}

EvaluationResult::EvaluationResult(
    uint64_t version,
    std::optional<uint64_t> flag_version,
    bool track_events,
    bool track_reason,
    std::optional<std::chrono::time_point<std::chrono::system_clock>>
        debug_events_until_date,
    EvaluationDetailInternal detail)
    : EvaluationResult(version,
                       flag_version,
                       track_events,
                       track_reason,
                       debug_events_until_date,
                       std::move(detail),
                       {}) {}

EvaluationResult::EvaluationResult(
    uint64_t version,
    std::optional<uint64_t> flag_version,
    bool track_events,
    bool track_reason,
    std::optional<std::chrono::time_point<std::chrono::system_clock>>
        debug_events_until_date,
    EvaluationDetailInternal detail,
    std::optional<std::vector<std::string>> prerequisites)
    : version_(version),
      flag_version_(flag_version),
      track_events_(track_events),
      track_reason_(track_reason),
      debug_events_until_date_(debug_events_until_date),
      detail_(std::move(detail)),
      prerequisites_(std::move(prerequisites)) {}

std::ostream& operator<<(std::ostream& out, EvaluationResult const& result) {
    out << "{";
    out << " version: " << result.Version();
    out << " trackEvents: " << result.TrackEvents();
    out << " trackReason: " << result.TrackReason();

    if (result.DebugEventsUntilDate().has_value()) {
        std::time_t as_time_t = std::chrono::system_clock::to_time_t(
            result.DebugEventsUntilDate().value());
        out << " debugEventsUntilDate: "
            << std::put_time(std::gmtime(&as_time_t), "%Y-%m-%d %H:%M:%S");
    }
    out << " detail: " << result.Detail();
    if (auto const prerequisites = result.Prerequisites()) {
        out << " prerequisites: [";
        for (std::size_t i = 0; i < prerequisites->size(); i++) {
            out << prerequisites->at(i)
                << (i == prerequisites->size() - 1 ? "" : ", ");
        }
        out << "]";
    }
    out << "}";
    return out;
}

bool operator==(EvaluationResult const& lhs, EvaluationResult const& rhs) {
    return lhs.Version() == rhs.Version() &&
           lhs.TrackReason() == rhs.TrackReason() &&
           lhs.TrackEvents() == rhs.TrackEvents() &&
           lhs.Detail() == rhs.Detail() &&
           lhs.DebugEventsUntilDate() == rhs.DebugEventsUntilDate() &&
           lhs.FlagVersion() == rhs.FlagVersion() &&
           lhs.Prerequisites() == rhs.Prerequisites();
}

bool operator!=(EvaluationResult const& lhs, EvaluationResult const& rhs) {
    return !(lhs == rhs);
}
}  // namespace launchdarkly
