#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace launchdarkly::server_side::integrations {

/**
 * @brief Membership describes which Big Segments a single context belongs to,
 * as reported by an @ref IBigSegmentStore lookup.
 *
 * Implementations of @ref IBigSegmentStore should construct a Membership using
 * @ref Membership::FromSegmentRefs and return it from @ref
 * IBigSegmentStore::GetMembership. The returned Membership is an immutable
 * snapshot — once constructed it does not observe any later changes to the
 * underlying store.
 *
 * A "segment ref" is the string `<segmentKey>.g<generation>`; the SDK
 * constructs that string when evaluating a flag and passes it to @ref
 * CheckMembership.
 */
class Membership {
   public:
    /**
     * @brief Constructs a Membership from the lists of segment refs the
     * context is included in / excluded from.
     *
     * If the same segment ref appears in both lists, inclusion wins, matching
     * the LaunchDarkly Big Segments spec (and the Java/Go SDK behavior).
     *
     * @param included_segment_refs Segment refs the context is explicitly
     * included in.
     * @param excluded_segment_refs Segment refs the context is explicitly
     * excluded from.
     */
    static Membership FromSegmentRefs(
        std::vector<std::string> const& included_segment_refs,
        std::vector<std::string> const& excluded_segment_refs);

    /**
     * @brief Returns the membership state for a single segment ref.
     *
     * @param segment_ref The `<segmentKey>.g<generation>` ref to look up.
     * @return `true` if the context is included, `false` if excluded, and
     * `std::nullopt` if the segment ref has no entry in this membership.
     */
    [[nodiscard]] std::optional<bool> CheckMembership(
        std::string const& segment_ref) const;

    Membership() = default;

   private:
    explicit Membership(std::unordered_map<std::string, bool> entries);

    // segment-ref → true (included) / false (excluded). Inclusion is the
    // stored value when a ref appears in both lists at construction time.
    std::unordered_map<std::string, bool> entries_;
};

/**
 * @brief Metadata describing the Big Segments store as a whole. Used to
 * detect staleness independent of any single context's membership.
 */
struct StoreMetadata {
    /**
     * @brief Wall-clock timestamp (Unix epoch) at which the data populator
     * (e.g. the LaunchDarkly Relay Proxy) last confirmed it had pushed all
     * pending Big Segments updates to the store.
     */
    std::chrono::milliseconds last_up_to_date;
};

}  // namespace launchdarkly::server_side::integrations
