#pragma once

#include <launchdarkly/server_side/integrations/big_segments/big_segment_store_types.hpp>

#include <tl/expected.hpp>

#include <optional>
#include <string>

namespace launchdarkly::server_side::integrations {

/**
 * @brief Interface for a Big Segments persistent store.
 *
 * A Big Segment is a segment whose membership list lives outside the
 * LaunchDarkly flag payload, in an external shared store populated by the
 * LaunchDarkly Relay Proxy. At evaluation time the SDK does a point lookup
 * against the store rather than carrying the full membership list in memory.
 *
 * Implementations of this interface live in dedicated integration libraries
 * (e.g. `server-sdk-redis-source`, `server-sdk-dynamodb-source`) and are
 * passed to the SDK via the Big Segments config builder. The SDK wraps every
 * implementation in an internal caching / staleness-tracking layer, so
 * implementations should NOT cache results themselves — perform every lookup
 * the SDK asks for.
 *
 * The SDK hashes the context key (SHA-256 then base64-encoded) before
 * calling @ref GetMembership, so an implementation only ever sees opaque
 * hashes; raw context keys are never sent to the store.
 *
 * Implementations must be thread-safe.
 */
class IBigSegmentStore {
   public:
    virtual ~IBigSegmentStore() = default;
    IBigSegmentStore(IBigSegmentStore const&) = delete;
    IBigSegmentStore(IBigSegmentStore&&) = delete;
    IBigSegmentStore& operator=(IBigSegmentStore const&) = delete;
    IBigSegmentStore& operator=(IBigSegmentStore&&) = delete;

    using GetMembershipResult = tl::expected<Membership, std::string>;
    using GetMetadataResult =
        tl::expected<std::optional<StoreMetadata>, std::string>;

    /**
     * @brief Looks up the Big Segments membership for a single context.
     *
     * @param context_hash Base64-encoded SHA-256 of the context key.
     * Implementations should treat this as an opaque identifier.
     *
     * @return A @ref Membership snapshot. Most contexts are in no Big
     * Segments, in which case the returned Membership has no entries (every
     * @ref Membership::CheckMembership call against it returns
     * `std::nullopt`). Returns an error if the lookup itself failed.
     */
    [[nodiscard]] virtual GetMembershipResult GetMembership(
        std::string const& context_hash) const = 0;

    /**
     * @brief Returns store-level metadata used by the SDK to detect staleness.
     *
     * @return @ref StoreMetadata if the store has a metadata record,
     * `std::nullopt` if no metadata has ever been written (the store was
     * never populated), or an error if the lookup itself failed.
     */
    [[nodiscard]] virtual GetMetadataResult GetMetadata() const = 0;

   protected:
    IBigSegmentStore() = default;
};

}  // namespace launchdarkly::server_side::integrations
