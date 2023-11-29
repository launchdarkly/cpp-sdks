#include <launchdarkly/server_side/integrations/redis/redis_source.hpp>

#include <sw/redis++/redis++.h>

namespace launchdarkly::server_side::data_systems {

std::string RedisDataSource::key_for_kind(
    integrations::ISerializedItemKind const& kind) const {
    return prefix_ + ":" + kind.Namespace();
}

RedisDataSource::RedisDataSource(std::string uri, std::string prefix)
    : prefix_(std::move(prefix)),
      inited_key_(prefix_ + ":$inited"),
      redis_(std::make_unique<sw::redis::Redis>(std::move(uri))) {}

RedisDataSource::~RedisDataSource() = default;

data_interfaces::ISerializedDataReader::GetResult RedisDataSource::Get(
    integrations::ISerializedItemKind const& kind,
    std::string const& itemKey) const {
    if (auto maybe_item = redis_->hget(key_for_kind(kind), itemKey)) {
        return integrations::SerializedItemDescriptor{0, false,
                                                      maybe_item.value()};
    }
    return tl::make_unexpected(Error{"not found"});
}

data_interfaces::ISerializedDataReader::AllResult RedisDataSource::All(
    integrations::ISerializedItemKind const& kind) const {
    std::unordered_map<std::string, std::string> raw_items;
    AllResult::value_type items;
    redis_->hgetall(key_for_kind(kind),
                    std::inserter(raw_items, raw_items.begin()));
    for (auto const& [key, val] : raw_items) {
        items.emplace(key,
                      integrations::SerializedItemDescriptor{0, false, val});
    }
    return items;
}

std::string const& RedisDataSource::Identity() const {
    static std::string const identity = "redis";
    return identity;
}

bool RedisDataSource::Initialized() const {
    return redis_->exists(inited_key_) == 1;
}

}  // namespace launchdarkly::server_side::data_systems
