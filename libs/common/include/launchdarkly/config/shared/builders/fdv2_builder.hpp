#pragma once

#include <launchdarkly/config/shared/built/fdv2_config.hpp>
#include <launchdarkly/config/shared/connection_mode.hpp>
#include <launchdarkly/config/shared/sdks.hpp>

#include <chrono>
#include <optional>
#include <string>

namespace launchdarkly::config::shared::builders {

/**
 * Contains methods for configuring the FDv2 data system, which determines how
 * the SDK obtains flag data and how it keeps that data current.
 *
 * A default-constructed builder is the configuration the SDK uses when the
 * application customizes nothing. It starts in streaming mode, and provides
 * streaming, polling, and offline modes.
 */
class FDv2Builder {
   public:
    using Config = built::FDv2Config<ClientSDK>;

    /**
     * Configures the local cache as a source. It has no options.
     *
     * Reading persisted flag data lets evaluation begin before the network
     * answers.
     */
    class Cache {
       public:
        /**
         * Build the cache config. Used internal to the SDK.
         * @return The built config.
         */
        [[nodiscard]] Config::CacheConfig Build() const;
    };

    /**
     * Contains methods for configuring a streaming source, which receives
     * updates as the service publishes them.
     */
    class Streaming {
       public:
        /**
         * Sets the initial reconnect delay for the streaming connection.
         *
         * The delay for the first reconnection will start near this value,
         * and then increase exponentially for any subsequent connection
         * failures.
         *
         * @param delay The initial delay for a reconnection attempt.
         * @return Reference to this builder.
         */
        Streaming& InitialReconnectDelay(std::chrono::milliseconds delay);

        /**
         * Sets the base URL for this source alone.
         *
         * Requests from this source are sent to the given URL instead of the
         * streaming URL the rest of the SDK uses. This is useful for routing
         * one tier to different infrastructure, such as a Relay Proxy used
         * only as a fallback.
         *
         * @param base_url The base URL for this source.
         * @return Reference to this builder.
         */
        Streaming& BaseUrl(std::string base_url);

        /**
         * Build the streaming config. Used internal to the SDK.
         * @return The built config.
         */
        [[nodiscard]] Config::StreamingConfig Build() const;

       private:
        std::chrono::milliseconds initial_reconnect_delay_{1000};
        std::optional<std::string> base_url_override_;
    };

    /**
     * Contains methods for configuring a polling source, which asks the
     * service for updates on an interval.
     */
    class Polling {
       public:
        /**
         * Sets the interval at which the SDK will poll for feature flag
         * updates.
         *
         * Intervals shorter than the minimum the SDK permits are raised to
         * it.
         *
         * @param interval The polling interval.
         * @return Reference to this builder.
         */
        Polling& PollInterval(std::chrono::seconds interval);

        /**
         * Sets the base URL for this source alone.
         *
         * Requests from this source are sent to the given URL instead of the
         * polling URL the rest of the SDK uses.
         *
         * @param base_url The base URL for this source.
         * @return Reference to this builder.
         */
        Polling& BaseUrl(std::string base_url);

        /**
         * Build the polling config. Used internal to the SDK.
         * @return The built config.
         */
        [[nodiscard]] Config::PollingConfig Build() const;

       private:
        std::chrono::seconds poll_interval_{std::chrono::minutes(5)};
        std::optional<std::string> base_url_override_;
    };

    /**
     * Contains methods for configuring the FDv1 polling source the SDK uses
     * while the service has directed it away from FDv2.
     *
     * The SDK returns to FDv2 on its own once the service's fallback period
     * has elapsed.
     */
    class FDv1Polling {
       public:
        /**
         * Sets the interval at which the SDK will poll while on FDv1.
         * @param interval The polling interval.
         * @return Reference to this builder.
         */
        FDv1Polling& PollInterval(std::chrono::seconds interval);

        /**
         * Sets the base URL for the fallback alone.
         *
         * Requests from the fallback are sent to the given URL instead of the
         * polling URL the rest of the SDK uses.
         *
         * @param base_url The base URL for the fallback.
         * @return Reference to this builder.
         */
        FDv1Polling& BaseUrl(std::string base_url);

        /**
         * Build the FDv1 fallback config. Used internal to the SDK.
         * @return The built config.
         */
        [[nodiscard]] Config::FDv1FallbackConfig Build() const;

       private:
        std::chrono::seconds poll_interval_{std::chrono::minutes(5)};
        std::optional<std::string> base_url_override_;
    };

    /**
     * Contains methods for configuring what one connection mode does, meaning
     * which sources load flag data and which keep it current.
     *
     * A mode built this way replaces the SDK's built-in definition entirely,
     * so it should list every source the mode needs, cache included.
     */
    class Mode {
       public:
        /**
         * Appends a source to the list that loads flag data.
         *
         * The sources run in order until one loads a complete data set.
         *
         * @param source The cache source.
         * @return Reference to this builder.
         */
        Mode& Initializer(Cache source);

        /**
         * Appends a source to the list that loads flag data.
         *
         * The sources run in order until one loads a complete data set.
         *
         * @param source The polling source.
         * @return Reference to this builder.
         */
        Mode& Initializer(Polling source);

        /**
         * Appends a source to the list that keeps flag data current.
         *
         * Order is preference. The first entry is the primary, and the SDK
         * falls back to later entries when it cannot keep the primary
         * running.
         *
         * @param source The streaming source.
         * @return Reference to this builder.
         */
        Mode& Synchronizer(Streaming source);

        /**
         * Appends a source to the list that keeps flag data current.
         *
         * Order is preference. The first entry is the primary, and the SDK
         * falls back to later entries when it cannot keep the primary
         * running.
         *
         * @param source The polling source.
         * @return Reference to this builder.
         */
        Mode& Synchronizer(Polling source);

        /**
         * Sets the FDv1 source to use if the service directs the SDK to it.
         * @param source The FDv1 fallback source.
         * @return Reference to this builder.
         */
        Mode& FDv1Fallback(FDv1Polling source);

        /**
         * Leaves the mode with no FDv1 source.
         *
         * A fallback directive then stops the mode's synchronizer until the
         * SDK returns to FDv2.
         *
         * @return Reference to this builder.
         */
        Mode& DisableFDv1Fallback();

        /**
         * Build the mode definition. Used internal to the SDK.
         * @return The built config.
         */
        [[nodiscard]] Config::ModeDefinition Build() const;

       private:
        Config::ModeDefinition definition_;
    };

    FDv2Builder();

    /**
     * Sets the mode the SDK starts in. Defaults to streaming.
     * @param mode The initial connection mode.
     * @return Reference to this builder.
     */
    FDv2Builder& InitialMode(ConnectionMode mode);

    /**
     * Replaces what the given mode does.
     *
     * Modes left uncustomized keep their built-in definitions.
     *
     * @param mode The connection mode to replace.
     * @param definition What the mode does.
     * @return Reference to this builder.
     */
    FDv2Builder& CustomizeMode(ConnectionMode mode, Mode definition);

    /**
     * Whether to send the evaluation context in a request body rather than
     * encoded into the request path.
     *
     * This keeps the context out of URL-based request logs and CDN logs, at
     * the cost of CDN caching.
     *
     * @param use_post True to send the context in a request body.
     * @return Reference to this builder.
     */
    FDv2Builder& UsePost(bool use_post);

    /**
     * Build the FDv2 config. Used internal to the SDK.
     * @return The built config.
     */
    [[nodiscard]] Config Build() const;

   private:
    Config config_;
};

}  // namespace launchdarkly::config::shared::builders
