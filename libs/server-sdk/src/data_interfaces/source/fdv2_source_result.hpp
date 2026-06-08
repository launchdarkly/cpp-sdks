#pragma once

#include "../item_change.hpp"

#include <launchdarkly/data_model/change_set.hpp>
#include <launchdarkly/data_sources/data_source_status_error_info.hpp>

#include <charconv>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <variant>

namespace launchdarkly::server_side::data_interfaces {

/**
 * Server-directed instruction to fall back to FDv1, carried alongside any
 * FDv2 source result whose underlying transport observed it (e.g. an
 * X-LD-FD-Fallback: true response header).
 */
struct FDv1FallbackDirective {
    /** Default TTL used when the directive carries no TTL of its own. */
    static constexpr std::chrono::seconds kDefaultTtl = std::chrono::hours(1);

    /**
     * Parse the value of an X-LD-FD-Fallback-TTL header (or equivalent
     * protocolFallbackTTL field from a goodbye message). Returns
     * std::nullopt if the value is malformed.
     */
    static std::optional<std::chrono::seconds> ParseTtl(
        std::string_view value) {
        std::uint64_t seconds = 0;
        auto const* begin = value.data();
        auto const* end = begin + value.size();
        auto const [ptr, ec] = std::from_chars(begin, end, seconds);
        if (ec != std::errc{} || ptr != end) {
            return std::nullopt;
        }
        return std::chrono::seconds(seconds);
    }

    /**
     * How long to stay on FDv1 before attempting to recover to FDv2.
     * A value of 0 seconds means the SDK should remain on FDv1
     * indefinitely.
     */
    std::chrono::seconds ttl = kDefaultTtl;
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
