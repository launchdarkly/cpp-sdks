#pragma once

#include <cstddef>
#include <optional>
#include <ostream>
#include <string>

namespace launchdarkly {

/**
 * Describes the reason that a flag evaluation produced a particular value.
 */
class EvaluationReason {
   public:
    enum class Kind {
        // The flag was off and therefore returned its configured off value.
        kOff = 0,
        // The flag was on but the context did not match any targets or rules.
        kFallthrough = 1,
        // The context key was specifically targeted for this flag.
        kTargetMatch = 2,
        // The context matched one of the flag's rules.
        kRuleMatch = 3,
        // The flag was considered off because it had at least one prerequisite
        // flag that either was off or did not return the desired variation.
        kPrerequisiteFailed = 4,
        // The flag could not be evaluated, e.g. because it does not exist or
        // due to an unexpected error.
        kError = 5
    };
    friend std::ostream& operator<<(std::ostream& out, Kind const& kind);

    /**
     * @return The general category of the reason.
     */
    [[nodiscard]] Kind const& kind() const;

    /**
     * A further description of the error condition, if the kind was `"ERROR"`.
     */
    [[nodiscard]] std::optional<std::string> error_kind() const;

    /**
     * The index of the matched rule (0 for the first), if the kind was
     * `"RULE_MATCH"`.
     */
    [[nodiscard]] std::optional<std::size_t> rule_index() const;

    /**
     * The unique identifier of the matched rule, if the kind was
     * `"RULE_MATCH"`.
     */
    [[nodiscard]] std::optional<std::string> rule_id() const;

    /**
     * The key of the failed prerequisite flag, if the kind was
     * `"PREREQUISITE_FAILED"`.
     */
    [[nodiscard]] std::optional<std::string> prerequisite_key() const;

    /**
     * Whether the evaluation was part of an experiment.
     *
     * This is true if the evaluation resulted in an experiment rollout and
     * served one of the variations in the experiment. Otherwise it is false or
     * undefined.
     */
    [[nodiscard]] bool in_experiment() const;

    /**
     * Describes the validity of Big Segment information, if and only if the
     * flag evaluation required querying at least one Big Segment.
     *
     * - `"HEALTHY"`: The Big Segment query involved in the flag evaluation was
     * successful, and the segment state is considered up to date.
     * - `"STALE"`: The Big Segment query involved in the flag evaluation was
     * successful, but the segment state may not be up to date
     * - `"NOT_CONFIGURED"`: Big Segments could not be queried for the flag
     * evaluation because the SDK configuration did not include a Big Segment
     * store.
     * - `"STORE_ERROR"`: The Big Segment query involved in the flag evaluation
     * failed, for instance due to a database error.
     */
    [[nodiscard]] std::optional<std::string> big_segment_status() const;

    EvaluationReason(Kind kind,
                     std::optional<std::string> error_kind,
                     std::optional<std::size_t> rule_index,
                     std::optional<std::string> rule_id,
                     std::optional<std::string> prerequisite_key,
                     bool in_experiment,
                     std::optional<std::string> big_segment_status);

    explicit EvaluationReason(std::string error_kind);

    friend std::ostream& operator<<(std::ostream& out,
                                    EvaluationReason const& reason);

   private:
    Kind kind_;
    std::optional<std::string> error_kind_;
    std::optional<std::size_t> rule_index_;
    std::optional<std::string> rule_id_;
    std::optional<std::string> prerequisite_key_;
    bool in_experiment_;
    std::optional<std::string> big_segment_status_;
};

bool operator==(EvaluationReason const& lhs, EvaluationReason const& rhs);
bool operator!=(EvaluationReason const& lhs, EvaluationReason const& rhs);

}  // namespace launchdarkly
