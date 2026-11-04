#pragma once

#include <launchdarkly/config/shared/built/fdv2_config.hpp>
#include <launchdarkly/config/shared/connection_mode.hpp>
#include <launchdarkly/config/shared/sdks.hpp>

#include <chrono>
#include <optional>
#include <string>

namespace launchdarkly::config::shared::builders {

/**
 * Configures the FDv2 data system: how the SDK obtains flag data, and how it
 * keeps that data current.
 *
 * A default-constructed builder is the configuration the SDK uses when the
 * application customizes nothing. It starts in streaming mode, and provides
 * streaming, polling, and offline modes.
 */
class FDv2Builder {
   public:
    using Config = built::FDv2Config<ClientSDK>;

    /**
     * Configures the local cache as a source. Reading persisted flag data
     * lets evaluation begin before the network answers. It has no options.
     */
    class Cache {
       public:
        [[nodiscard]] Config::CacheConfig Build() const;
    };

    /**
     * Configures a streaming source, which receives updates as the service
     * publishes them.
     */
    class Streaming {
       public:
        /**
         * Sets where the reconnection backoff starts. The delay for the
         * first reconnection starts near this value and grows exponentially
         * for subsequent failures.
         */
        Streaming& InitialReconnectDelay(std::chrono::milliseconds delay);

        /**
         * Sends this source's requests to the given URL instead of the
         * streaming URL the rest of the SDK uses. Useful for routing one tier
         * to different infrastructure, such as a Relay Proxy used only as a
         * fallback.
         */
        Streaming& BaseUrl(std::string base_url);

        [[nodiscard]] Config::StreamingConfig Build() const;

       private:
        std::chrono::milliseconds initial_reconnect_delay_{1000};
        std::optional<std::string> base_url_override_;
    };

    /**
     * Configures a polling source, which asks the service for updates on an
     * interval.
     */
    class Polling {
       public:
        /**
         * Sets how long to wait between polls. Intervals shorter than the
         * minimum the SDK permits are raised to it.
         */
        Polling& PollInterval(std::chrono::seconds interval);

        /**
         * Sends this source's requests to the given URL instead of the
         * polling URL the rest of the SDK uses.
         */
        Polling& BaseUrl(std::string base_url);

        [[nodiscard]] Config::PollingConfig Build() const;

       private:
        std::chrono::seconds poll_interval_{std::chrono::minutes(5)};
        std::optional<std::string> base_url_override_;
    };

    /**
     * Configures the FDv1 polling source the SDK uses while the service has
     * directed it away from FDv2. The SDK returns to FDv2 on its own once the
     * service's fallback period has elapsed.
     */
    class FDv1Fallback {
       public:
        /** Sets how long to wait between polls while on FDv1. */
        FDv1Fallback& PollInterval(std::chrono::seconds interval);

        /**
         * Sends the fallback's requests to the given URL instead of the
         * polling URL the rest of the SDK uses.
         */
        FDv1Fallback& BaseUrl(std::string base_url);

        [[nodiscard]] Config::FDv1FallbackConfig Build() const;

       private:
        std::chrono::seconds poll_interval_{std::chrono::minutes(5)};
        std::optional<std::string> base_url_override_;
    };

    /**
     * Configures what one connection mode does: which sources load flag data,
     * and which keep it current.
     *
     * A mode built this way replaces the SDK's built-in definition entirely,
     * so it should list every source the mode needs, cache included.
     */
    class Mode {
       public:
        /**
         * Appends a source to the list that runs, in order, until one loads a
         * complete data set.
         */
        Mode& Initializer(Cache source);
        Mode& Initializer(Polling source);

        /**
         * Appends a source to the list that keeps data current. Order is
         * preference. The first entry is the primary, and the SDK falls back
         * to later entries when it cannot keep the primary running.
         */
        Mode& Synchronizer(Streaming source);
        Mode& Synchronizer(Polling source);

        /** Sets the FDv1 source to use if the service directs the SDK to it. */
        Mode& FallbackToFDv1(FDv1Fallback source);

        /**
         * Leaves the mode with no FDv1 source. A fallback directive then
         * stops the mode's synchronizer until the SDK returns to FDv2.
         */
        Mode& DisableFDv1Fallback();

        [[nodiscard]] Config::ModeDefinition Build() const;

       private:
        Config::ModeDefinition definition_;
    };

    FDv2Builder();

    /**
     * Sets the mode the SDK starts in. Defaults to streaming.
     */
    FDv2Builder& InitialMode(ConnectionMode mode);

    /**
     * Replaces what the given mode does. Modes left uncustomized keep their
     * built-in definitions.
     */
    FDv2Builder& CustomizeMode(ConnectionMode mode, Mode definition);

    /**
     * Sends the evaluation context in a request body rather than encoded into
     * the request path. This keeps the context out of URL-based request logs
     * and CDN logs, at the cost of CDN caching.
     */
    FDv2Builder& UsePost(bool use_post);

    /**
     * Builds the FDv2 config. Used internal to the SDK.
     */
    [[nodiscard]] Config Build() const;

   private:
    Config config_;
};

}  // namespace launchdarkly::config::shared::builders
