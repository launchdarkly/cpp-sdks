#pragma once

#include <launchdarkly/server_side/all_flags_state.hpp>
#include <launchdarkly/server_side/client.hpp>

#include "../data_store/data_store.hpp"
#include "../evaluation/evaluator.hpp"

namespace launchdarkly::server_side {

bool IsExperimentationEnabled(data_model::Flag const& flag,
                              std::optional<EvaluationReason> const& reason);

bool IsSet(AllFlagsStateOptions options, AllFlagsStateOptions flag);
bool NotSet(AllFlagsStateOptions options, AllFlagsStateOptions flag);

class AllFlagsStateBuilder {
   public:
    /**
     * Constructs a builder capable of generating a AllFlagsState structure.
     * @param options Options affecting the behavior of the builder.
     */
    AllFlagsStateBuilder(enum AllFlagsStateOptions options);

    /**
     * Adds a flag, including its evaluation result and additional state.
     * @param key Key of the flag.
     * @param value Value of the flag.
     * @param state State of the flag.
     */
    void AddFlag(std::string const& key,
                 Value value,
                 AllFlagsState::Metadata state);

    /**
     * Builds a AllFlagsState structure from the flags added to the builder.
     * This operation consumes the builder, and must only be called once.
     * @return
     */
    [[nodiscard]] AllFlagsState Build();

   private:
    enum AllFlagsStateOptions options_;
    std::unordered_map<std::string, AllFlagsState::Metadata> flags_state_;
    std::unordered_map<std::string, Value> evaluations_;
};
}  // namespace launchdarkly::server_side
