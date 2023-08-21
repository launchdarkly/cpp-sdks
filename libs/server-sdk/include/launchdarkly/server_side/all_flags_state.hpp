#pragma once

#include <launchdarkly/data/evaluation_reason.hpp>
#include <launchdarkly/value.hpp>

#include <optional>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side {

class AllFlagsState {
   public:
    class Metadata {
       public:
        Metadata(std::uint64_t version,
                 std::optional<std::int64_t> variation,
                 std::optional<EvaluationReason> reason,
                 bool track_events,
                 bool track_reason,
                 std::optional<std::uint64_t> debug_events_until_date);

        [[nodiscard]] std::uint64_t Version() const;
        [[nodiscard]] std::optional<std::int64_t> Variation() const;
        [[nodiscard]] std::optional<EvaluationReason> const& Reason() const;
        [[nodiscard]] bool TrackEvents() const;
        [[nodiscard]] bool TrackReason() const;
        [[nodiscard]] std::optional<std::uint64_t> const& DebugEventsUntilDate()
            const;
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

    [[nodiscard]] bool Valid() const;

    [[nodiscard]] std::unordered_map<std::string, Metadata> const& FlagsState()
        const;
    [[nodiscard]] std::unordered_map<std::string, Value> const& Evaluations()
        const;

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
                  std::unordered_map<std::string, Metadata> flags_state);

   private:
    bool const valid_;
    const std::unordered_map<std::string, Metadata> flags_state_;
    const std::unordered_map<std::string, Value> evaluations_;
};

bool operator==(AllFlagsState::Metadata const& lhs,
                AllFlagsState::Metadata const& rhs);

bool operator==(AllFlagsState const& lhs, AllFlagsState const& rhs);

}  // namespace launchdarkly::server_side
