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
 */
void ExecuteAfterTrack(std::vector<std::shared_ptr<Hook>> const& hooks,
                       TrackSeriesContext const& context,
                       const Logger& logger);

}  // namespace launchdarkly::server_side::hooks
