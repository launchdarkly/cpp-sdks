#pragma once

#include <launchdarkly/data_model/fdv2_change.hpp>
#include <launchdarkly/data_sources/data_source_status_error_info.hpp>

#include <optional>
#include <string>
#include <variant>

namespace launchdarkly::server_side::data_interfaces {

/**
 * Result returned by IFDv2Initializer::Run and IFDv2Synchronizer::Next.
 *
 * Mirrors Java's FDv2SourceResult.
 */
struct FDv2SourceResult {
    using ErrorInfo = common::data_sources::DataSourceStatusErrorInfo;

    /**
     * A changeset was successfully received and is ready to apply.
     */
    struct ChangeSet {
        data_model::FDv2ChangeSet change_set;
        /** If true, the server signaled that the client should fall back to
         * FDv1. */
        bool fdv1_fallback;
    };

    /**
     * A transient error occurred; the source may recover.
     */
    struct Interrupted {
        ErrorInfo error;
        bool fdv1_fallback;
    };

    /**
     * A non-recoverable error occurred; the source should not be retried.
     */
    struct TerminalError {
        ErrorInfo error;
        bool fdv1_fallback;
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
        bool fdv1_fallback;
    };

    using Value =
        std::variant<ChangeSet, Interrupted, TerminalError, Shutdown, Goodbye>;

    Value value;
};

}  // namespace launchdarkly::server_side::data_interfaces
