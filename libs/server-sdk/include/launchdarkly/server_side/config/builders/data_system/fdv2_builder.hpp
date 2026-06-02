#pragma once

#include <launchdarkly/config/shared/builders/data_source_builder.hpp>
#include <launchdarkly/config/shared/sdks.hpp>
#include <launchdarkly/server_side/config/built/data_system/fdv2_config.hpp>

#include <chrono>

namespace launchdarkly::server_side::config::builders {

class FDv2Builder {
   public:
    using StreamingSource =
        launchdarkly::config::shared::builders::StreamingBuilder<
            launchdarkly::config::shared::ServerSDK>;
    using PollingSource =
        launchdarkly::config::shared::builders::PollingBuilder<
            launchdarkly::config::shared::ServerSDK>;

    FDv2Builder();

    /**
     * @brief Configures the primary FDv2 streaming synchronizer.
     * Defaults to the standard FDv2 streaming endpoint with no payload filter.
     * @param source Streaming source configuration.
     * @return Reference to this.
     */
    FDv2Builder& Streaming(StreamingSource source);

    /**
     * @brief Configures the secondary FDv2 polling synchronizer used as a
     * fallback when streaming is unavailable.
     * @param source Polling source configuration.
     * @return Reference to this.
     */
    FDv2Builder& Polling(PollingSource source);

    /**
     * @brief Configures the FDv1 streaming source used as a last-resort
     * fallback when the LaunchDarkly service signals (via the
     * X-LD-FD-Fallback header) that the SDK should switch to FDv1. Enabled
     * by default with standard settings.
     * @param source Streaming source configuration to use for the FDv1
     *     fallback connection.
     * @return Reference to this.
     */
    FDv2Builder& FDv1Fallback(StreamingSource source);

    /**
     * @brief Disables the FDv1 streaming fallback. After this call, an
     * FDv1 fallback directive from the service transitions the SDK to
     * OFFLINE rather than reconnecting via FDv1.
     * @return Reference to this.
     */
    FDv2Builder& DisableFDv1Fallback();

    /**
     * @brief Sets how long the active synchronizer may remain interrupted
     * before the orchestrator falls back to the next-preferred synchronizer.
     * @param timeout Duration the synchronizer must be continuously
     *     interrupted for before fallback fires.
     * @return Reference to this.
     */
    FDv2Builder& FallbackTimeout(std::chrono::milliseconds timeout);

    /**
     * @brief Sets how long a fallback synchronizer must run successfully
     * before the orchestrator attempts to recover to the primary
     * synchronizer.
     * @param timeout Duration the fallback synchronizer must run before a
     *     recovery attempt is made.
     * @return Reference to this.
     */
    FDv2Builder& RecoveryTimeout(std::chrono::milliseconds timeout);

    [[nodiscard]] built::FDv2Config Build() const;

   private:
    built::FDv2Config config_;
};

}  // namespace launchdarkly::server_side::config::builders
