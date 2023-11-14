#pragma once

#include "../../../../data_interfaces/source/iserialized_pull_source.hpp"

namespace launchdarkly::server_side::data_systems {

class RedisDataSource final
    : public data_interfaces::ISerializedDataPullSource {
   public:
    RedisDataSource();
    [[nodiscard]] GetResult Get(integrations::IPersistentKind const& kind,
                                std::string const& itemKey) const override;
    [[nodiscard]] AllResult All(
        integrations::IPersistentKind const& kind) const override;
    [[nodiscard]] std::string const& Identity() const override;
    [[nodiscard]] bool Initialized() const override;
};

}  // namespace launchdarkly::server_side::data_systems
