#pragma once

#include <launchdarkly/config/shared/builders/data_source_builder.hpp>
#include <launchdarkly/config/shared/sdks.hpp>
#include <launchdarkly/server_side/config/built/data_system/fdv2_config.hpp>

#include <chrono>

namespace launchdarkly::server_side::config::builders {

class FDv2Builder {
   public:
    using Streaming = launchdarkly::config::shared::builders::StreamingBuilder<
        launchdarkly::config::shared::ServerSDK>;
    using Polling = launchdarkly::config::shared::builders::PollingBuilder<
        launchdarkly::config::shared::ServerSDK>;

    FDv2Builder();

    /**
     * @brief Appends a polling initializer to the initializers list. The
     * first call to this method on a default-constructed builder replaces
     * the spec-default initializer list; subsequent calls append.
     * @param source Polling source configuration for the initializer.
     * @return Reference to this.
     */
    FDv2Builder& Initializer(Polling source);

    /**
     * @brief Appends a streaming synchronizer to the synchronizers list.
     * Order in the list determines preference: the first entry is the
     * primary synchronizer, subsequent entries are fallbacks. The first
     * call to a Synchronizer overload on a default-constructed builder
     * replaces the spec-default synchronizer list; subsequent calls append.
     * @param source Streaming source configuration.
     * @return Reference to this.
     */
    FDv2Builder& Synchronizer(Streaming source);

    /**
     * @brief Appends a polling synchronizer to the synchronizers list. See
     * Synchronizer(Streaming) for ordering and default-replacement
     * semantics.
     * @param source Polling source configuration.
     * @return Reference to this.
     */
    FDv2Builder& Synchronizer(Polling source);

    /**
     * @brief Configures the FDv1 streaming source used as a last-resort
     * fallback when the LaunchDarkly service signals (via the
     * X-LD-FD-Fallback header) that the SDK should switch to FDv1.
     * Enabled by default with standard streaming settings.
     * @param source Streaming source configuration to use for the FDv1
     *     fallback connection.
     * @return Reference to this.
     */
    FDv2Builder& FDv1Fallback(Streaming source);

    /**
     * @brief Configures the FDv1 polling source used as a last-resort
     * fallback when the LaunchDarkly service signals (via the
     * X-LD-FD-Fallback header) that the SDK should switch to FDv1.
     * @param source Polling source configuration to use for the FDv1
     *     fallback connection.
     * @return Reference to this.
     */
    FDv2Builder& FDv1Fallback(Polling source);

    /**
     * @brief Disables the FDv1 fallback. After this call, an FDv1
     * fallback directive from the service transitions the SDK to
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
    bool initializers_explicit_;
    bool synchronizers_explicit_;
};

}  // namespace launchdarkly::server_side::config::builders
