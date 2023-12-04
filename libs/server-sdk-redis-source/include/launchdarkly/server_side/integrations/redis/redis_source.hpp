#pragma once

#include <launchdarkly/server_side/integrations/data_reader/iserialized_data_reader.hpp>

#include <tl/expected.hpp>

#include <memory>
#include <string>

namespace sw::redis {
class Redis;
}

namespace launchdarkly::server_side::integrations {

class RedisDataSource final : public ISerializedDataReader {
   public:
    static tl::expected<std::shared_ptr<RedisDataSource>, std::string> Create(
        std::string uri,
        std::string prefix);

    [[nodiscard]] GetResult Get(ISerializedItemKind const& kind,
                                std::string const& itemKey) const override;
    [[nodiscard]] AllResult All(
        integrations::ISerializedItemKind const& kind) const override;
    [[nodiscard]] std::string const& Identity() const override;
    [[nodiscard]] bool Initialized() const override;

    ~RedisDataSource();

   private:
    RedisDataSource(std::unique_ptr<sw::redis::Redis> redis,
                    std::string prefix);

    std::string key_for_kind(ISerializedItemKind const& kind) const;

    std::string const prefix_;
    std::string const inited_key_;
    std::unique_ptr<sw::redis::Redis> redis_;
};

}  // namespace launchdarkly::server_side::integrations
