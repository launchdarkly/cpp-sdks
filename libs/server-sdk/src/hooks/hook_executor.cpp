#include "hook_executor.hpp"

#include <exception>

namespace launchdarkly::server_side::hooks {

EvaluationSeriesExecutor::EvaluationSeriesExecutor(
    std::vector<std::shared_ptr<Hook>> const& hooks,
    Logger& logger)
    : hooks_(hooks), logger_(logger) {
    // Initialize series data storage for each hook
    series_data_.resize(hooks_.size());
}

void EvaluationSeriesExecutor::BeforeEvaluation(
    EvaluationSeriesContext const& context) {
    // Execute hooks in order of registration
    for (std::size_t i = 0; i < hooks_.size(); ++i) {
        try {
            series_data_[i] =
                hooks_[i]->BeforeEvaluation(context, series_data_[i]);
        } catch (std::exception const& e) {
            LogHookError("BeforeEvaluation",
                         std::string(hooks_[i]->Metadata().Name()),
                         std::string(context.FlagKey()), e.what());
            // On error, use data from previous successful stage
            // (already stored in series_data_[i])
        } catch (...) {
            LogHookError("BeforeEvaluation",
                         std::string(hooks_[i]->Metadata().Name()),
                         std::string(context.FlagKey()), "unknown error");
        }
    }
}

void EvaluationSeriesExecutor::AfterEvaluation(
    EvaluationSeriesContext const& context,
    EvaluationDetail<Value> const& detail) {
    // Execute hooks in reverse order of registration
    for (std::size_t i = hooks_.size(); i-- > 0;) {
        try {
            series_data_[i] =
                hooks_[i]->AfterEvaluation(context, series_data_[i], detail);
        } catch (std::exception const& e) {
            LogHookError("AfterEvaluation",
                         std::string(hooks_[i]->Metadata().Name()),
                         std::string(context.FlagKey()), e.what());
            // On error, use data from previous successful stage
        } catch (...) {
            LogHookError("AfterEvaluation",
                         std::string(hooks_[i]->Metadata().Name()),
                         std::string(context.FlagKey()), "unknown error");
        }
    }
}

void EvaluationSeriesExecutor::LogHookError(
    std::string const& stage,
    std::string const& hook_name,
    std::string const& flag_name,
    std::string const& error_message) {
    LD_LOG(logger_, LogLevel::kError)
        << "[hooks] During evaluation of flag \"" << flag_name << "\", stage \""
        << stage << "\" of hook \"" << hook_name
        << "\" reported error: " << error_message;
}

void ExecuteAfterTrack(std::vector<std::shared_ptr<Hook>> const& hooks,
                       TrackSeriesContext const& context,
                       Logger& logger) {
    // Execute hooks in order of registration
    for (auto const& hook : hooks) {
        try {
            hook->AfterTrack(context);
        } catch (std::exception const& e) {
            LD_LOG(logger, LogLevel::kError)
                << "[hooks] During track of event \"" << context.Key()
                << "\", stage \"AfterTrack\" of hook \""
                << hook->Metadata().Name()
                << "\" reported error: " << e.what();
        } catch (...) {
            LD_LOG(logger, LogLevel::kError)
                << "[hooks] During track of event \"" << context.Key()
                << "\", stage \"AfterTrack\" of hook \""
                << hook->Metadata().Name() << "\" reported error: unknown error";
        }
    }
}

}  // namespace launchdarkly::server_side::hooks
