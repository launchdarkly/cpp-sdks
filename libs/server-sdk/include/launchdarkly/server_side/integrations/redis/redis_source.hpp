#pragma once

#include <launchdarkly/server_side/data_interfaces/sources/iserialized_pull_source.hpp>

#include <sw/redis++/redis++.h>

namespace launchdarkly::server_side::data_systems {

class RedisDataSource final
    : public data_interfaces::ISerializedDataPullSource {
   public:
    RedisDataSource(std::string uri, std::string prefix);
    [[nodiscard]] GetResult Get(integrations::ISerializedItemKind const& kind,
                                std::string const& itemKey) const override;
    [[nodiscard]] AllResult All(
        integrations::ISerializedItemKind const& kind) const override;
    [[nodiscard]] std::string const& Identity() const override;
    [[nodiscard]] bool Initialized() const override;

   private:
    std::string const prefix_;
    std::string const inited_key_;
    std::string key_for_kind(integrations::ISerializedItemKind const& kind) const;
    mutable sw::redis::Redis redis_;
};

}  // namespace launchdarkly::server_side::data_systems
