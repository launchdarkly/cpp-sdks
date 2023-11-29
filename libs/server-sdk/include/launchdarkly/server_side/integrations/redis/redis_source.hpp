#pragma once

#include <launchdarkly/server_side/data_interfaces/sources/iserialized_data_reader.hpp>

#include <tl/expected.hpp>

#include <string>

namespace sw::redis {
class Redis;
}

namespace launchdarkly::server_side::data_systems {

class RedisDataSource final : public data_interfaces::ISerializedDataReader {
   public:

    static tl::expected<std::shared_ptr<RedisDataSource>, std::string> Create(std::string uri, std::string prefix);

    [[nodiscard]] GetResult Get(integrations::ISerializedItemKind const& kind,
                                std::string const& itemKey) const override;
    [[nodiscard]] AllResult All(
        integrations::ISerializedItemKind const& kind) const override;
    [[nodiscard]] std::string const& Identity() const override;
    [[nodiscard]] bool Initialized() const override;

    ~RedisDataSource();

   private:
    RedisDataSource(std::unique_ptr<sw::redis::Redis> redis, std::string prefix);

    std::string const prefix_;
    std::string const inited_key_;
    std::string key_for_kind(
        integrations::ISerializedItemKind const& kind) const;
    std::unique_ptr<sw::redis::Redis> redis_;
};

}  // namespace launchdarkly::server_side::data_systems
