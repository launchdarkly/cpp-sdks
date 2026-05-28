/** @file redis_big_segment_store.hpp
 * @brief Server-Side Redis Big Segments Store
 */

#pragma once

#include <launchdarkly/server_side/integrations/big_segments/ibig_segment_store.hpp>

#include <tl/expected.hpp>

#include <memory>
#include <string>

namespace sw::redis {
class Redis;
}

namespace launchdarkly::server_side::integrations {

/**
 * @brief RedisBigSegmentStore is a Big Segments persistent store backed by
 * Redis.
 *
 * Call RedisBigSegmentStore::Create to obtain a new instance, then pass it to
 * the SDK via the Big Segments config builder.
 *
 * The same Redis database can be shared with @ref RedisDataSource — Big
 * Segments keys use their own prefixed namespaces (`big_segment_include`,
 * `big_segment_exclude`, `big_segments_synchronized_on`) and do not conflict
 * with the flag/segment hashes. The LaunchDarkly Relay Proxy is responsible
 * for populating Big Segments data in Redis; this class only reads from it.
 *
 * This implementation is backed by <a
 * href="https://github.com/sewenew/redis-plus-plus">Redis++</a>, a C++ wrapper
 * for the <a href="https://github.com/redis/hiredis">hiredis</a> library.
 */
class RedisBigSegmentStore final : public IBigSegmentStore {
   public:
    /**
     * @brief Creates a new RedisBigSegmentStore, or returns an error if
     * construction failed.
     *
     * @param uri Redis URI. The URI is passed to the underlying Redis++ client
     * verbatim. See <a
     * href="https://github.com/sewenew/redis-plus-plus#api-reference">Redis++
     * API Reference</a> for details on the possible URI formats.
     *
     * @param prefix Prefix to use when reading SDK data from Redis. This
     * allows multiple LaunchDarkly environments to be stored in the same
     * database (under different prefixes).
     *
     * @return A RedisBigSegmentStore, or an error if construction failed.
     */
    static tl::expected<std::unique_ptr<RedisBigSegmentStore>, std::string>
    Create(std::string uri, std::string prefix);

    [[nodiscard]] GetMembershipResult GetMembership(
        std::string const& context_hash) const override;
    [[nodiscard]] GetMetadataResult GetMetadata() const override;

    ~RedisBigSegmentStore() override;

   private:
    RedisBigSegmentStore(std::unique_ptr<sw::redis::Redis> redis,
                         std::string prefix);

    std::unique_ptr<sw::redis::Redis> redis_;
    std::string const include_key_prefix_;
    std::string const exclude_key_prefix_;
    std::string const sync_time_key_;
};

}  // namespace launchdarkly::server_side::integrations
