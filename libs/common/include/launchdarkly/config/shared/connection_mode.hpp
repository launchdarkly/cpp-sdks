#pragma once

namespace launchdarkly::config::shared {

/**
 * A named data system configuration: which sources the SDK uses to load flag
 * data, and which it uses to keep that data current.
 */
enum class ConnectionMode {
    /** Stream updates, falling back to polling. */
    kStreaming,
    /** Poll for updates on an interval. */
    kPolling,
    /** Evaluate against whatever is cached, and make no requests. */
    kOffline,
};

/**
 * The mode's name as the configuration API spells it.
 */
char const* GetConnectionModeName(ConnectionMode mode);

}  // namespace launchdarkly::config::shared
