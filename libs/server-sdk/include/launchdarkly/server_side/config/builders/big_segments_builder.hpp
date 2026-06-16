#pragma once

#include <launchdarkly/server_side/config/built/big_segments_config.hpp>
#include <launchdarkly/server_side/integrations/big_segments/ibig_segment_store.hpp>

#include <launchdarkly/error.hpp>

#include <tl/expected.hpp>

#include <chrono>
#include <cstddef>
#include <memory>

namespace launchdarkly::server_side::config::builders {

/**
 * @brief Configures the SDK's Big Segments behavior.
 *
 * Not thread-safe. Construct, configure, and call @ref Build on a single
 * thread; the resulting @ref built::BigSegmentsConfig is safe to share.
 */
class BigSegmentsBuilder {
   public:
    /**
     * @brief Constructs a builder for the given Big Segments store.
     *
     * @param store The Big Segments store implementation. Shared ownership;
     * the SDK retains a reference for the lifetime of the client.
     */
    explicit BigSegmentsBuilder(
        std::shared_ptr<integrations::IBigSegmentStore> store);

    /**
     * @brief Sets the maximum number of context membership lookups cached
     * by the SDK. Defaults to 1000.
     *
     * To reduce store traffic, the SDK maintains an LRU cache keyed by
     * context key. A higher value reduces store queries for
     * recently-referenced contexts at the cost of memory.
     */
    BigSegmentsBuilder& ContextCacheSize(std::size_t size);

    /**
     * @brief Sets the time-to-live for cached membership lookups. Defaults
     * to 5 seconds.
     *
     * A higher value reduces store queries for any given context, but
     * delays the SDK noticing membership changes. Zero or negative
     * durations are coerced to the default.
     */
    BigSegmentsBuilder& ContextCacheTime(std::chrono::milliseconds ttl);

    /**
     * @brief Sets the interval at which the SDK polls the store's metadata
     * to determine availability and staleness. Defaults to 5 seconds.
     *
     * Zero or negative durations are coerced to the default.
     */
    BigSegmentsBuilder& StatusPollInterval(std::chrono::milliseconds interval);

    /**
     * @brief Sets how long the SDK waits before treating store data as
     * stale. Defaults to 2 minutes.
     *
     * If the store's last-updated timestamp falls behind the current time
     * by more than this duration, evaluations report a big segments status
     * of `STALE` and the status provider reports the store as stale. Zero
     * or negative durations are coerced to the default.
     */
    BigSegmentsBuilder& StaleAfter(std::chrono::milliseconds threshold);

    /**
     * @brief Resolves the configuration.
     *
     * Returns an error if the store passed to the constructor was null.
     *
     * If the configured @ref StatusPollInterval exceeds @ref StaleAfter,
     * the poll interval in the returned config is clamped to the
     * stale-after value so the SDK can detect staleness within one poll
     * cycle.
     */
    [[nodiscard]] tl::expected<built::BigSegmentsConfig, Error> Build() const;

   private:
    std::shared_ptr<integrations::IBigSegmentStore> store_;
    std::size_t context_cache_size_;
    std::chrono::milliseconds context_cache_time_;
    std::chrono::milliseconds status_poll_interval_;
    std::chrono::milliseconds stale_after_;
};

}  // namespace launchdarkly::server_side::config::builders
