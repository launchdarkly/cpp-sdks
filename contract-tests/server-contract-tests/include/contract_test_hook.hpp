#pragma once

#include <data_model/data_model.hpp>
#include <launchdarkly/server_side/hooks/hook.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <memory>
#include <string>

/**
 * ContractTestHook implements the Hook interface for contract testing.
 *
 * It posts hook execution payloads to a callback URI specified in the test
 * configuration, allowing the test harness to verify hook behavior.
 */
class ContractTestHook : public launchdarkly::server_side::hooks::Hook {
   public:
    /**
     * Constructs a contract test hook.
     * @param executor IO executor for async HTTP operations.
     * @param config Hook configuration from the test harness.
     */
    ContractTestHook(boost::asio::any_io_executor executor,
                     ConfigHookInstance config);

    ~ContractTestHook() override = default;

    [[nodiscard]] launchdarkly::server_side::hooks::HookMetadata const&
    Metadata() const override;

    launchdarkly::server_side::hooks::EvaluationSeriesData BeforeEvaluation(
        launchdarkly::server_side::hooks::EvaluationSeriesContext const&
            series_context,
        launchdarkly::server_side::hooks::EvaluationSeriesData data) override;

    launchdarkly::server_side::hooks::EvaluationSeriesData AfterEvaluation(
        launchdarkly::server_side::hooks::EvaluationSeriesContext const&
            series_context,
        launchdarkly::server_side::hooks::EvaluationSeriesData data,
        launchdarkly::EvaluationDetail<launchdarkly::Value> const& detail)
        override;

    void AfterTrack(launchdarkly::server_side::hooks::TrackSeriesContext const&
                        series_context) override;

   private:
    /**
     * Posts a hook execution payload to the callback URI.
     * @param stage The stage being executed.
     * @param payload The JSON payload to send.
     */
    void PostCallback(std::string const& stage, nlohmann::json const& payload);

    /**
     * Gets configured data for a specific stage.
     * @param stage The stage name.
     * @return Optional map of key-value pairs for this stage.
     */
    std::optional<std::unordered_map<std::string, nlohmann::json>> GetDataForStage(
        std::string const& stage) const;

    /**
     * Gets configured error for a specific stage.
     * @param stage The stage name.
     * @return Optional error message for this stage.
     */
    std::optional<std::string> GetErrorForStage(std::string const& stage) const;

    boost::asio::any_io_executor executor_;
    ConfigHookInstance config_;
    launchdarkly::server_side::hooks::HookMetadata metadata_;
};
