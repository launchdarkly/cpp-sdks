#pragma once

#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_detail.hpp>
#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/value.hpp>

#include "bucketing.hpp"
#include "detail/evaluation_stack.hpp"
#include "evaluation_error.hpp"

#include "../data_interfaces/store/istore.hpp"
#include "../events/event_scope.hpp"

#include <tl/expected.hpp>

namespace launchdarkly::server_side::evaluation {

class Evaluator {
   public:
    Evaluator(Logger& logger, data_interfaces::IStore const& source);

    /**
     * Evaluates a flag for a given context.
     * Warning: not thread safe.
     *
     * @param flag The flag to evaluate.
     * @param context The context to evaluate the flag against.
     * @param event_scope The event scope used for recording prerequisite
     * events.
     */
    [[nodiscard]] EvaluationDetail<Value> Evaluate(
        data_model::Flag const& flag,
        Context const& context,
        EventScope const& event_scope);

    /**
     * Evaluates a flag for a given context. Does not record prerequisite
     * events. Warning: not thread safe.
     *
     * @param flag The flag to evaluate.
     * @param context The context to evaluate the flag against.
     */
    [[nodiscard]] EvaluationDetail<Value> Evaluate(data_model::Flag const& flag,
                                                   Context const& context);

   private:
    [[nodiscard]] EvaluationDetail<Value> Evaluate(
        std::optional<std::string> parent_key,
        data_model::Flag const& flag,
        Context const& context,
        EventScope const& event_scope);

    [[nodiscard]] EvaluationDetail<Value> FlagVariation(
        data_model::Flag const& flag,
        data_model::Flag::Variation variation_index,
        EvaluationReason reason) const;

    [[nodiscard]] EvaluationDetail<Value> OffValue(
        data_model::Flag const& flag,
        EvaluationReason reason) const;

    void LogError(std::string const& key, Error const& error) const;

    Logger& logger_;
    data_interfaces::IStore const& source_;
    detail::EvaluationStack stack_;
};
}  // namespace launchdarkly::server_side::evaluation
