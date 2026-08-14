#pragma once

namespace launchdarkly::server_side::data_components {

/**
 * @brief Trustworthiness of a Big Segments membership answer, surfaced on an
 * evaluation's reason.
 *
 * - kHealthy: the store was queried successfully and its data is up to date.
 * - kStale: queried successfully, but the data may be out of date.
 * - kStoreError: the store query failed.
 * - kNotConfigured: a flag referenced a Big Segment but no store is wired up,
 *   or the segment has no generation.
 */
enum class BigSegmentsStatus { kHealthy, kStale, kStoreError, kNotConfigured };

/**
 * @brief Health of the Big Segments store as a whole, independent of any single
 * context's membership.
 */
struct BigSegmentStoreStatus {
    // The most recent store query or metadata poll succeeded.
    bool available{false};
    // The store's last-known update is older than the configured threshold.
    bool stale{false};
};

inline bool operator==(BigSegmentStoreStatus const& a,
                       BigSegmentStoreStatus const& b) {
    return a.available == b.available && a.stale == b.stale;
}

inline bool operator!=(BigSegmentStoreStatus const& a,
                       BigSegmentStoreStatus const& b) {
    return !(a == b);
}

}  // namespace launchdarkly::server_side::data_components
