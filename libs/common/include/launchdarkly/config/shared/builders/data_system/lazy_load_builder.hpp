#pragma once

#include <launchdarkly/config/shared/builders/data_source_builder.hpp>
#include <launchdarkly/config/shared/built/data_system/lazy_load_config.hpp>
#include <launchdarkly/config/shared/sdks.hpp>
#include <launchdarkly/error.hpp>

#include <chrono>

namespace launchdarkly::config::shared::builders {

/**
 * \brief LazyLoadBuilder allows for specifying the configuration of
 * the Lazy Load data system, which is appropriate when a LaunchDarkly
 * environment should be stored external to the SDK (such as in Redis.)
 *
 * In the Lazy Load system, flag and segment data is fetched on-demand from the
 * database and stored in an in-memory cache for a specific duration. This
 * allows the SDK to maintain a working set of data that may be a specific
 * subset of the entire environment.
 *
 * The database is read-only from the perspective of the SDK. To populate the
 * database with flag and segment data, an external process (e.g. Relay Proxy or
 * another SDK) is necessary.
 */
struct LazyLoadBuilder {
    using Redis = RedisPullBuilder;
    using EvictionPolicy = built::LazyLoadConfig::EvictionPolicy;
    /**
     * \brief Constructs a new LazyLoadBuilder.
     */
    LazyLoadBuilder();

    /**
     * \brief Specify the source of the data. Currently, only Redis is
     * supported.
     * \param source The Redis configuration.
     * \return Reference to this.
     */
    LazyLoadBuilder& Source(Redis source);

    /**
     * \brief
     * \param ttl Specify the duration data items should be live in-memory
     * before being refreshed from the database. The chosen \ref EvictionPolicy
     * affects usage of this TTL. \return Reference to this.
     */
    LazyLoadBuilder& CacheTTL(std::chrono::milliseconds ttl);

    /**
     * \brief Specify the eviction policy when a data item's TTL expires.
     * At this time, only EvictionPolicy::Disabled is supported (the default),
     * which leaves stale items in the cache until they can be refreshed. \param
     * policy The EvictionPolicy. \return Reference to this.
     */
    LazyLoadBuilder& CacheEviction(EvictionPolicy policy);

    [[nodiscard]] tl::expected<built::LazyLoadConfig, Error> Build() const;

   private:
    RedisPullBuilder redis_builder_;
    built::LazyLoadConfig config_;
};

}  // namespace launchdarkly::config::shared::builders
