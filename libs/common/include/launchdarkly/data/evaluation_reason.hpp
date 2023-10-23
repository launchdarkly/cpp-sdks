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
    /**
     * Do not change these values. They must remain stable for the C API.
     */
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
     * Do not change these values. They must remain stable for the C API.
     */
    enum class ErrorKind {
        // The SDK was not yet fully initialized and cannot evaluate flags.
        kClientNotReady = 0,
        // The application did not pass valid context attributes to the SDK
        // evaluation method.
        kUserNotSpecified = 1,
        // No flag existed with the specified flag key.
        kFlagNotFound = 2,
        // The application requested an evaluation result of one type but the
        // resulting flag variation value was of a different type.
        kWrongType = 3,
        // The flag had invalid properties.
        kMalformedFlag = 4,
        // An unexpected error happened that stopped evaluation.
        kException = 5,
    };

    friend std::ostream& operator<<(std::ostream& out, ErrorKind const& kind);

    /**
     * @return The general category of the reason.
     */
    [[nodiscard]] enum Kind const& Kind() const;

    /**
     * A further description of the error condition, if the Kind was
     * Kind::kError.
     */
    [[nodiscard]] std::optional<ErrorKind> ErrorKind() const;

    /**
     * The index of the matched rule (0 for the first), if the kind was
     * `"RULE_MATCH"`.
     */
    [[nodiscard]] std::optional<std::size_t> RuleIndex() const;

    /**
     * The unique identifier of the matched rule, if the kind was
     * `"RULE_MATCH"`.
     */
    [[nodiscard]] std::optional<std::string> RuleId() const;

    /**
     * The key of the failed prerequisite flag, if the kind was
     * `"PREREQUISITE_FAILED"`.
     */
    [[nodiscard]] std::optional<std::string> PrerequisiteKey() const;

    /**
     * Whether the evaluation was part of an experiment.
     *
     * This is true if the evaluation resulted in an experiment rollout and
     * served one of the variations in the experiment. Otherwise it is false or
     * undefined.
     */
    [[nodiscard]] bool InExperiment() const;

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
    [[nodiscard]] std::optional<std::string> BigSegmentStatus() const;

    EvaluationReason(enum Kind kind,
                     std::optional<enum ErrorKind> error_kind,
                     std::optional<std::size_t> rule_index,
                     std::optional<std::string> rule_id,
                     std::optional<std::string> prerequisite_key,
                     bool in_experiment,
                     std::optional<std::string> big_segment_status);

    explicit EvaluationReason(enum ErrorKind error_kind);

    /**
     * The flag was off.
     */
    static EvaluationReason Off();

    /**
     * The flag didn't return a variation due to a prerequisite failing.
     */
    static EvaluationReason PrerequisiteFailed(std::string prerequisite_key);

    /**
     * The flag evaluated to a particular variation due to a target match.
     */
    static EvaluationReason TargetMatch();

    /**
     * The flag evaluated to its fallthrough value.
     * @param in_experiment Whether the flag is part of an experiment.
     */
    static EvaluationReason Fallthrough(bool in_experiment);

    /**
     * The flag evaluated to a particular variation because it matched a rule.
     * @param rule_index Index of the rule.
     * @param rule_id ID of the rule.
     * @param in_experiment Whether the flag is part of an experiment.
     */
    static EvaluationReason RuleMatch(std::size_t rule_index,
                                      std::optional<std::string> rule_id,
                                      bool in_experiment);

    /**
     * The flag data was malformed.
     */
    static EvaluationReason MalformedFlag();

    friend std::ostream& operator<<(std::ostream& out,
                                    EvaluationReason const& reason);

   private:
    enum Kind kind_;
    std::optional<enum ErrorKind> error_kind_;
    std::optional<std::size_t> rule_index_;
    std::optional<std::string> rule_id_;
    std::optional<std::string> prerequisite_key_;
    bool in_experiment_;
    std::optional<std::string> big_segment_status_;
};

bool operator==(EvaluationReason const& lhs, EvaluationReason const& rhs);
bool operator!=(EvaluationReason const& lhs, EvaluationReason const& rhs);

}  // namespace launchdarkly
