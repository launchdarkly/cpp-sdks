#pragma once

#include <launchdarkly/server_side/client.hpp>

#include <launchdarkly/value.hpp>

#include <string>
#include <unordered_map>

namespace launchdarkly::server_side {

bool IsSet(AllFlagsState::Options options, AllFlagsState::Options flag);
bool NotSet(AllFlagsState::Options options, AllFlagsState::Options flag);

class AllFlagsStateBuilder {
   public:
    /**
     * Constructs a builder capable of generating a AllFlagsState structure.
     * @param options Options affecting the behavior of the builder.
     */
    explicit AllFlagsStateBuilder(AllFlagsState::Options options);

    /**
     * Adds a flag, including its evaluation result and additional state.
     * @param key Key of the flag.
     * @param value Value of the flag.
     * @param state State of the flag.
     */
    void AddFlag(std::string const& key,
                 Value value,
                 AllFlagsState::State state);

    /**
     * Builds a AllFlagsState structure from the flags added to the builder.
     * This operation consumes the builder, and must only be called once.
     * @return
     */
    [[nodiscard]] AllFlagsState Build();

   private:
    AllFlagsState::Options options_;
    std::unordered_map<std::string, AllFlagsState::State> flags_state_;
    std::unordered_map<std::string, Value> evaluations_;
};
}  // namespace launchdarkly::server_side
