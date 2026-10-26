#pragma once

#include "../item_change.hpp"

#include <launchdarkly/data_model/change_set.hpp>
#include <launchdarkly/data_sources/data_source_status_error_info.hpp>

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace launchdarkly::server_side::data_interfaces {

/**
 * Server-directed instruction to fall back to FDv1, carried alongside any
 * FDv2 source result whose underlying transport observed it (e.g. an
 * X-LD-FD-Fallback: true response header).
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
     * Builds a directive from the raw value of an X-LD-FD-Fallback-TTL
     * header. A value that is not a whole number of seconds uses the
     * jittered default.
     */
    static FDv1FallbackDirective FromServiceTtl(std::string_view ttl);

    /** How long to stay off FDv2 before attempting to recover to it. */
    std::chrono::seconds ttl;
};

/**
 * Result returned by IFDv2Initializer::Run and IFDv2Synchronizer::Next.
 */
struct FDv2SourceResult {
    using ErrorInfo = common::data_sources::DataSourceStatusErrorInfo;

    /**
     * A changeset was successfully received and is ready to apply.
     */
    struct ChangeSet {
        data_model::ChangeSet<ChangeSetData> change_set;
    };

    /**
     * A transient error occurred; the source may recover.
     */
    struct Interrupted {
        ErrorInfo error;
    };

    /**
     * A non-recoverable error occurred; the source should not be retried.
     */
    struct TerminalError {
        ErrorInfo error;
    };

    /**
     * The source was closed cleanly (via Close()).
     */
    struct Shutdown {};

    /**
     * The server sent a goodbye; the orchestrator should rotate sources.
     */
    struct Goodbye {
        std::optional<std::string> reason;
    };

    using Value =
        std::variant<ChangeSet, Interrupted, TerminalError, Shutdown, Goodbye>;

    Value value;

    /**
     * Set if the underlying transport observed an FDv1 fallback directive.
     */
    std::optional<FDv1FallbackDirective> fdv1_fallback;
};

}  // namespace launchdarkly::server_side::data_interfaces
