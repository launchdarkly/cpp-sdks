#pragma once

#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/server_side/hooks/hook.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace launchdarkly::server_side::hooks {

/**
 * Executes hooks for a single evaluation series.
 * Each instance handles one evaluation from beforeEvaluation through
 * afterEvaluation.
 *
 * Handles exceptions/errors from hooks and logs them without affecting
 * the evaluation operation.
 *
 * THREADING AND LIFETIME GUARANTEES:
 * - All hook callbacks are executed SYNCHRONOUSLY in the same thread as the
 *   caller of the SDK method (e.g., BoolVariation, Track).
 * - Hook callbacks complete before control returns to the caller.
 * - Context objects (EvaluationSeriesContext, TrackSeriesContext) passed to
 *   hooks are stack-allocated and remain valid for the entire duration of
 *   the callback.
 * - This synchronous execution model is CRITICAL for the C bindings, which
 *   return pointers to internal data that are only valid during the callback.
 *   Asynchronous execution would create dangling pointers.
 * - Hook implementations MUST NOT store references, pointers, or string_views
 *   from context objects beyond the immediate callback execution.
 *
 * IMPLEMENTATION REQUIREMENTS:
 * - Do NOT introduce async hook execution without redesigning the C bindings.
 * - Do NOT move hook execution to background threads.
 * - If async hooks are needed in the future, the C bindings must be changed
 *   to return heap-allocated copies instead of pointers to stack data.
 */
class EvaluationSeriesExecutor {
   public:
    /**
     * Constructs an evaluation series executor.
     * @param hooks The list of hooks to execute.
     * @param logger Logger for reporting hook errors.
     */
    EvaluationSeriesExecutor(std::vector<std::shared_ptr<Hook>> const& hooks,
                             Logger& logger);

    /**
     * Executes beforeEvaluation stages for all hooks in order of registration.
     * @param context The evaluation context.
     */
    void BeforeEvaluation(EvaluationSeriesContext const& context);

    /**
     * Executes afterEvaluation stages for all hooks in reverse order of
     * registration.
     * @param context The evaluation context.
     * @param detail The evaluation result.
     */
    void AfterEvaluation(EvaluationSeriesContext const& context,
                         EvaluationDetail<Value> const& detail);

   private:
    std::vector<std::shared_ptr<Hook>> const& hooks_;
    Logger& logger_;

    // Per-invocation series data for each hook.
    // The outer vector index corresponds to hook index.
    std::vector<EvaluationSeriesData> series_data_;

    void LogHookError(std::string const& stage,
                      std::string const& hook_name,
                      std::string const& flag_name,
                      std::string const& error_message) const;
};

/**
 * Utility for executing track hooks.
 *
 * THREADING AND LIFETIME GUARANTEES:
 * - Executes all hooks SYNCHRONOUSLY in the caller's thread.
 * - The TrackSeriesContext parameter must remain valid for the entire
 *   execution (typically stack-allocated by the caller).
 * - Returns only after all hook callbacks have completed.
 * - This synchronous model is REQUIRED for C binding safety - the C bindings
 *   return pointers to data owned by the context parameter.
 */
void ExecuteAfterTrack(std::vector<std::shared_ptr<Hook>> const& hooks,
                       TrackSeriesContext const& context,
                       const Logger& logger);

}  // namespace launchdarkly::server_side::hooks
