#pragma once

#include <launchdarkly/config/shared/builders/data_source_builder.hpp>
#include <launchdarkly/config/shared/sdks.hpp>
#include <launchdarkly/server_side/config/built/data_system/fdv2_config.hpp>

#include <chrono>
#include <optional>
#include <string>

namespace launchdarkly::server_side::config::builders {

class FDv2Builder {
   public:
    class Streaming {
       public:
        Streaming& InitialReconnectDelay(std::chrono::milliseconds delay);
        Streaming& Filter(std::string filter_key);
        Streaming& BaseUrl(std::string base_url);
        [[nodiscard]] built::FDv2Config::StreamingConfig Build() const;

       private:
        std::chrono::milliseconds initial_reconnect_delay_{1000};
        std::optional<std::string> filter_key_;
        std::optional<std::string> base_url_override_;
    };

    class Polling {
       public:
        Polling& PollInterval(std::chrono::seconds interval);
        Polling& Filter(std::string filter_key);
        Polling& BaseUrl(std::string base_url);
        [[nodiscard]] built::FDv2Config::PollingConfig Build() const;

       private:
        std::chrono::seconds poll_interval_{30};
        std::optional<std::string> filter_key_;
        std::optional<std::string> base_url_override_;
    };

    /**
     * Constructs a builder with no initializers, no synchronizers, and no
     * FDv1 fallback. Use Default() for the spec-recommended configuration.
     */
    FDv2Builder();

    /**
     * @return A builder pre-populated with the spec-recommended initializers,
     *     synchronizers, and FDv1 fallback. Equivalent to calling
     *     Initializer(), Synchronizer(), and FDv1Fallback() with the
     *     standard sources.
     */
    static FDv2Builder Default();

    /**
     * @brief Appends a polling initializer to the initializers list.
     * @param source Polling source configuration for the initializer.
     * @return Reference to this.
     */
    FDv2Builder& Initializer(Polling source);

    /**
     * @brief Appends a streaming synchronizer to the synchronizers list.
     * Order in the list determines preference: the first entry is the
     * primary synchronizer, subsequent entries are fallbacks.
     * @param source Streaming source configuration.
     * @return Reference to this.
     */
    FDv2Builder& Synchronizer(Streaming source);

    /**
     * @brief Appends a polling synchronizer to the synchronizers list. See
     * Synchronizer(Streaming) for ordering semantics.
     * @param source Polling source configuration.
     * @return Reference to this.
     */
    FDv2Builder& Synchronizer(Polling source);

    using FDv1Streaming =
        launchdarkly::config::shared::builders::StreamingBuilder<
            launchdarkly::config::shared::ServerSDK>;
    using FDv1Polling = launchdarkly::config::shared::builders::PollingBuilder<
        launchdarkly::config::shared::ServerSDK>;

    /**
     * @brief Configures the FDv1 streaming source used as a last-resort
     * fallback when the LaunchDarkly service signals (via the
     * X-LD-FD-Fallback header) that the SDK should switch to FDv1. The
     * fallback reads its endpoint from the top-level ServiceEndpoints; to
     * point the fallback at a custom URL, configure ServiceEndpoints
     * accordingly.
     * @param source FDv1 streaming source configuration.
     * @return Reference to this.
     */
    FDv2Builder& FDv1Fallback(FDv1Streaming source);

    /**
     * @brief Configures the FDv1 polling source used as a last-resort
     * fallback when the LaunchDarkly service signals (via the
     * X-LD-FD-Fallback header) that the SDK should switch to FDv1. The
     * fallback reads its endpoint from the top-level ServiceEndpoints; to
     * point the fallback at a custom URL, configure ServiceEndpoints
     * accordingly.
     * @param source FDv1 polling source configuration.
     * @return Reference to this.
     */
    FDv2Builder& FDv1Fallback(FDv1Polling source);

    /**
     * @brief Disables the FDv1 fallback. After this call, an FDv1
     * fallback directive from the service leaves the data source in
     * the interrupted state and schedules an FDv2 retry on the
     * directive's TTL.
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
