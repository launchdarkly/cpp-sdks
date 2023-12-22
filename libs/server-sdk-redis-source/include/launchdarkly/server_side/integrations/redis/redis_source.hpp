/** @file redis_source.hpp
 * @brief Server-Side Redis Source
 */

#pragma once

#include <launchdarkly/server_side/integrations/data_reader/iserialized_data_reader.hpp>

#include <tl/expected.hpp>

#include <memory>
#include <string>

namespace sw::redis {
class Redis;
}

namespace launchdarkly::server_side::integrations {
/**
 * @brief RedisDataSource represents a data source for the Server-Side SDK
 * backed by Redis. It is meant to be used in place of the standard
 * LaunchDarkly Streaming or Polling data sources.
 *
 * Call RedisDataSource::Create to obtain a new instance. This instance
 * can be passed into the SDK's DataSystem configuration via the LazyLoad
 * builder.
 *
 * This implementation is backed by <a
 * href="https://github.com/sewenew/redis-plus-plus">Redis++</a>, a C++ wrapper
 * for the <a href="https://github.com/redis/hiredis">hiredis</a> library.
 */
class RedisDataSource final : public ISerializedDataReader {
   public:
    /**
     * @brief Creates a new RedisDataSource, or returns an error if construction
     * failed.
     *
     * @param uri Redis URI. The URI is passed to the underlying Redis++ client
     * verbatim. See <a
     * href="https://github.com/sewenew/redis-plus-plus#api-reference">Redis++
     * API Reference</a> for details on the possible URI formats.
     *
     * @param prefix Prefix to use when reading SDK data from Redis. This allows
     * multiple LaunchDarkly environments to be stored in the same database
     * (under different prefixes.)
     *
     * @return A RedisDataSource, or an error if construction failed.
     */
    static tl::expected<std::unique_ptr<RedisDataSource>, std::string> Create(
        std::string uri,
        std::string prefix);

    [[nodiscard]] GetResult Get(ISerializedItemKind const& kind,
                                std::string const& itemKey) const override;
    [[nodiscard]] AllResult All(ISerializedItemKind const& kind) const override;
    [[nodiscard]] std::string const& Identity() const override;
    [[nodiscard]] bool Initialized() const override;

    ~RedisDataSource() override;  // = default

   private:
    RedisDataSource(std::unique_ptr<sw::redis::Redis> redis,
                    std::string prefix);

    [[nodiscard]] std::string key_for_kind(
        ISerializedItemKind const& kind) const;

    std::string const prefix_;
    std::string const inited_key_;
    std::unique_ptr<sw::redis::Redis> redis_;
};
}  // namespace launchdarkly::server_side::integrations
