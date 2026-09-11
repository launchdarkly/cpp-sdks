#pragma once

#include "../data_source_update_sink.hpp"

#include <launchdarkly/data_sources/data_source_status_error_info.hpp>

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace launchdarkly::client_side::data_sources {

/**
 * An instruction from the service to stop using FDv2 and fall back to FDv1.
 */
struct FDv1FallbackDirective {
    /** Used whenever the service supplies no usable TTL. */
    static constexpr std::chrono::seconds kDefaultTtl = std::chrono::hours(1);

    /**
     * Builds a directive using the jittered default TTL, so that SDKs which
     * fell back together do not all retry at once. Used when the service
     * supplies no usable TTL. Safe to call from any thread.
     */
    static FDv1FallbackDirective DefaultTtl();

    /**
     * Builds a directive from a service-supplied TTL, used as given. A value
     * outside (0, 1 hour] uses the jittered default instead, so a fallback is
     * never indefinite.
     */
    static FDv1FallbackDirective FromServiceTtl(std::chrono::seconds ttl);

    /**
     * Builds a directive from the raw value of a TTL response header. A value
     * that is not a whole number of seconds uses the default.
     */
    static FDv1FallbackDirective FromServiceTtl(std::string_view ttl);

    /** How long to stay off FDv2 before attempting to recover to it. */
    std::chrono::seconds ttl;
};

/**
 * What an initializer or synchronizer produced: either flag data to apply, or
 * a signal about the source's own state.
 */
struct FDv2SourceResult {
    using ErrorInfo = common::data_sources::DataSourceStatusErrorInfo;

    /** A changeset was received and is ready to apply. */
    struct ChangeSet {
        FlagChangeSet change_set;
    };

    /** A transient error occurred. The source may recover. */
    struct Interrupted {
        ErrorInfo error;
    };

    /** A non-recoverable error occurred. The source should not be retried. */
    struct TerminalError {
        ErrorInfo error;
    };

    /** The source was closed cleanly. */
    struct Shutdown {};

    /** The service sent a goodbye. The orchestrator should rotate sources. */
    struct Goodbye {
        std::optional<std::string> reason;
    };

    using Value =
        std::variant<ChangeSet, Interrupted, TerminalError, Shutdown, Goodbye>;

    Value value;

    /** Set if the underlying transport observed an FDv1 fallback directive. */
    std::optional<FDv1FallbackDirective> fdv1_fallback;

    /**
     * The environment the service reported this result was evaluated in, if
     * the underlying transport could observe it.
     */
    std::optional<std::string> environment_id;
};

}  // namespace launchdarkly::client_side::data_sources
