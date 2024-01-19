#pragma once

#include <launchdarkly/data/evaluation_reason.hpp>
#include <launchdarkly/value.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side {

/**
 * AllFlagsState is a snapshot of the state of multiple feature flags with
 * regard to a specific evaluation context.
 *
 * Serializing this object to JSON using boost::json::value_from will produce
 * the appropriate data structure for bootstrapping the LaunchDarkly JavaScript
 * client.
 *
 * To do this, the header
 * <launchdarkly/server_side/serialization/json_all_flags_state.hpp> must be
 * included to make the appropriate `tag_invoke` implementations available to
 * boost.
 */
class AllFlagsState {
   public:
    enum class Options : std::uint8_t {
        /**
         * Default behavior.
         */
        Default = 0,
        /**
         * Include evaluation reasons in the state object. By default, they
         * are not.
         */
        IncludeReasons = (1 << 0),
        /**
         * Include detailed flag metadata only for flags with event tracking
         * or debugging turned on.
         *
         * This reduces the size of the JSON data if you are
         * passing the flag state to the front end.
         */
        DetailsOnlyForTrackedFlags = (1 << 1),
        /**
         * Include only flags marked for use with the client-side SDK.
         * By default, all flags are included.
         */
        ClientSideOnly = (1 << 2)
    };

    /**
     * State contains information pertaining to a single feature flag.
     */
    class State {
       public:
        State(std::uint64_t version,
              std::optional<std::int64_t> variation,
              std::optional<EvaluationReason> reason,
              bool track_events,
              bool track_reason,
              std::optional<std::uint64_t> debug_events_until_date);

        /**
         * @return The flag's version number when it was evaluated.
         */
        [[nodiscard]] std::uint64_t Version() const;

        /**
         * @return The variation index that was selected for the specified
         * evaluation context.
         */
        [[nodiscard]] std::optional<std::int64_t> Variation() const;

        /**
         * @return The reason that the flag evaluation produced the specified
         * variation.
         */
        [[nodiscard]] std::optional<EvaluationReason> const& Reason() const;

        /**
         * @return True if a full feature event must be sent when evaluating
         * this flag. This will be true if tracking was explicitly enabled for
         * this flag for data export, or if the evaluation involved an
         * experiment, or both.
         */
        [[nodiscard]] bool TrackEvents() const;

        /**
         * @return True if the evaluation reason should always be included in
         * any full feature event created for this flag, regardless of whether a
         * VariationDetail method was called. This will be true if the
         * evaluation involved an experiment.
         */
        [[nodiscard]] bool TrackReason() const;

        /**
         * @return The date on which debug mode expires for this flag, if
         * enabled.
         */
        [[nodiscard]] std::optional<std::uint64_t> const& DebugEventsUntilDate()
            const;

        /**
         *
         * @return True if the options passed to AllFlagsState, combined with
         * the obtained flag state, indicate that some metadata can be left out
         * of the JSON serialization.
         */
        [[nodiscard]] bool OmitDetails() const;

        friend class AllFlagsStateBuilder;

       private:
        std::uint64_t version_;
        std::optional<std::int64_t> variation_;
        std::optional<EvaluationReason> reason_;
        bool track_events_;
        bool track_reason_;
        std::optional<std::uint64_t> debug_events_until_date_;
        bool omit_details_;
    };

    /**
     * @return True if the call to AllFlagsState succeeded. False if there was
     * an error, such as the data store being unavailable. When false, the other
     * accessors will return empty maps.
     */
    [[nodiscard]] bool Valid() const;

    /**
     * @return A map of metadata for each flag.
     */
    [[nodiscard]] std::unordered_map<std::string, State> const& States() const;

    /**
     * @return A map of evaluation results for each flag.
     */
    [[nodiscard]] std::unordered_map<std::string, Value> const& Values() const;

    /**
     * Constructs an invalid instance of AllFlagsState.
     */
    AllFlagsState();

    /**
     * Constructs a valid instance of AllFlagsState.
     * @param evaluations A map of evaluation results for each flag.
     * @param flags_state A map of metadata for each flag.
     */
    AllFlagsState(std::unordered_map<std::string, Value> evaluations,
                  std::unordered_map<std::string, class State> flags_state);

   private:
    bool const valid_;
    std::unordered_map<std::string, class State> const flags_state_;
    std::unordered_map<std::string, Value> const evaluations_;
};

void operator|=(AllFlagsState::Options& lhs, AllFlagsState::Options rhs);
AllFlagsState::Options operator|(AllFlagsState::Options lhs,
                                 AllFlagsState::Options rhs);

AllFlagsState::Options operator&(AllFlagsState::Options lhs,
                                 AllFlagsState::Options rhs);

bool operator==(class AllFlagsState::State const& lhs,
                class AllFlagsState::State const& rhs);

bool operator==(AllFlagsState const& lhs, AllFlagsState const& rhs);

}  // namespace launchdarkly::server_side
