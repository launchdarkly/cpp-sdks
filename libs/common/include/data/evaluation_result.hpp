#pragma once

#include <optional>

#include <boost/json/value.hpp>

#include "evaluation_detail.hpp"

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
    [[nodiscard]] uint64_t version() const;

    /**
     * Incremented by LaunchDarkly each time the flag's configuration changes.
     */
    [[nodiscard]] std::optional<uint64_t> flag_version() const;

    /**
     * True if a client SDK should track events for this flag.
     */
    [[nodiscard]] bool track_events() const;

    /**
     * True if a client SDK should track reasons for this flag.
     */
    [[nodiscard]] bool track_reason() const;

    /**
     * A timestamp, which if the current time is before, a client SDK
     * should send debug events for the flag.
     * @return
     */
    [[nodiscard]] std::optional<uint64_t> debug_events_until_date() const;

    /**
     * Details of the flags evaluation.
     */
    [[nodiscard]] EvaluationDetail const& detail() const;

    EvaluationResult(uint64_t version,
                     std::optional<uint64_t> flag_version,
                     bool track_events,
                     bool track_reason,
                     std::optional<long> debug_events_until_date,
                     EvaluationDetail detail);

   private:
    uint64_t version_;
    std::optional<uint64_t> flag_version_;
    bool track_events_;
    bool track_reason_;
    std::optional<uint64_t> debug_events_until_date_;
    EvaluationDetail detail_;
};

/**
 * Method used by boost::json for converting a boost::json::value into a
 * launchdarkly::EvaluationResult.
 * @return A EvaluationResult representation of the boost::json::value.
 */
EvaluationResult tag_invoke(
    boost::json::value_to_tag<EvaluationResult> const& unused,
    boost::json::value const& json_value);

}  // namespace launchdarkly
