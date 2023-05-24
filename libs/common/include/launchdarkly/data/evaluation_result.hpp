#pragma once

#include <chrono>
#include <optional>
#include <ostream>

#include <launchdarkly/data/evaluation_detail_internal.hpp>

namespace launchdarkly {

/**
 * FlagMeta represents an evaluated flag either from the LaunchDarkly service,
 * or in bootstrap data generated by a server SDK.
 */
class EvaluationResult {
   public:
    /**
     * Incremented by LaunchDarkly each time the flag's state changes.
     */
    [[nodiscard]] uint64_t Version() const;

    /**
     * Incremented by LaunchDarkly each time the flag's configuration changes.
     */
    [[nodiscard]] std::optional<uint64_t> FlagVersion() const;

    /**
     * True if a client SDK should track events for this flag.
     */
    [[nodiscard]] bool TrackEvents() const;

    /**
     * True if a client SDK should track reasons for this flag.
     */
    [[nodiscard]] bool TrackReason() const;

    /**
     * A timestamp, which if the current time is before, a client SDK
     * should send debug events for the flag.
     * @return
     */
    [[nodiscard]] std::optional<
        std::chrono::time_point<std::chrono::system_clock>>
    DebugEventsUntilDate() const;

    /**
     * Details of the flags evaluation.
     */
    [[nodiscard]] EvaluationDetailInternal const& Detail() const;

    EvaluationResult(
        uint64_t version,
        std::optional<uint64_t> flag_version,
        bool track_events,
        bool track_reason,
        std::optional<std::chrono::time_point<std::chrono::system_clock>>
            debug_events_until_date,
        EvaluationDetailInternal detail);

    friend std::ostream& operator<<(std::ostream& out,
                                    EvaluationResult const& result);

   private:
    uint64_t version_;
    std::optional<uint64_t> flag_version_;
    bool track_events_;
    bool track_reason_;
    std::optional<std::chrono::time_point<std::chrono::system_clock>>
        debug_events_until_date_;
    EvaluationDetailInternal detail_;
};

bool operator==(EvaluationResult const& lhs, EvaluationResult const& rhs);
bool operator!=(EvaluationResult const& lhs, EvaluationResult const& rhs);

}  // namespace launchdarkly
