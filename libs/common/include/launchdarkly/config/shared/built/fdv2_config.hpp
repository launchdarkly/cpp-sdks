#pragma once

#include <launchdarkly/config/shared/connection_mode.hpp>
#include <launchdarkly/config/shared/sdks.hpp>

#include <chrono>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace launchdarkly::config::shared::built {

template <typename SDK>
struct FDv2Config;

/**
 * The FDv2 data system configuration: which connection mode the SDK starts
 * in, what each mode's sources are, and the settings shared across them.
 */
template <>
struct FDv2Config<ClientSDK> {
    /** Reads flag data the SDK persisted on a previous run. */
    struct CacheConfig {};

    struct StreamingConfig {
        std::chrono::milliseconds initial_reconnect_delay;
        /** Overrides the streaming base URL for this source alone. */
        std::optional<std::string> base_url_override;
    };

    struct PollingConfig {
        std::chrono::seconds poll_interval;
        /** Overrides the polling base URL for this source alone. */
        std::optional<std::string> base_url_override;
    };

    /**
     * The FDv1 polling source used while the service has directed the SDK
     * away from FDv2.
     */
    struct FDv1FallbackConfig {
        std::chrono::seconds poll_interval;
        /** Overrides the polling base URL for the fallback alone. */
        std::optional<std::string> base_url_override;
    };

    using InitializerEntry = std::variant<CacheConfig, PollingConfig>;
    using SynchronizerEntry = std::variant<PollingConfig, StreamingConfig>;

    /**
     * What one connection mode does. The synchronizer list is ordered by
     * preference. The first entry is the primary, and later entries are the
     * tiers the SDK falls back to.
     */
    struct ModeDefinition {
        std::vector<InitializerEntry> initializers;
        std::vector<SynchronizerEntry> synchronizers;
        std::optional<FDv1FallbackConfig> fdv1_fallback;
    };

    /** The mode the SDK starts in. */
    ConnectionMode initial_mode;

    /**
     * Where a source sends its requests when it does not override the URL
     * itself. FDv2's endpoints are not the ones FDv1 uses, so these hold
     * FDv2's own defaults. When the application configures its own endpoints,
     * these are resolved to those instead.
     */
    std::string polling_base_url;
    std::string streaming_base_url;

    /** What each mode does. Modes absent from the map are unavailable. */
    std::map<ConnectionMode, ModeDefinition> modes;

    /**
     * Whether to send the evaluation context in a request body rather than
     * base64url-encoded into the request path.
     */
    bool use_post;

    /**
     * How long the active synchronizer may remain interrupted before the SDK
     * falls back to the next tier.
     */
    std::chrono::milliseconds fallback_timeout;

    /**
     * How long a fallback tier must run before the SDK attempts to return to
     * the preferred one.
     */
    std::chrono::milliseconds recovery_timeout;
};

}  // namespace launchdarkly::config::shared::built
