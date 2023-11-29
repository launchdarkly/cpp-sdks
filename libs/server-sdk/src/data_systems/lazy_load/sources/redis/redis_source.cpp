#include <launchdarkly/server_side/integrations/redis/redis_source.hpp>

#include <sw/redis++/redis++.h>

namespace launchdarkly::server_side::data_systems {

tl::expected<std::shared_ptr<RedisDataSource>, std::string>
RedisDataSource::Create(std::string uri, std::string prefix) {
    try {
        return std::shared_ptr<RedisDataSource>(new RedisDataSource(
            std::make_unique<sw::redis::Redis>(std::move(uri)),
            std::move(prefix)));
    } catch (sw::redis::Error const& e) {
        return tl::make_unexpected(e.what());
    }
}

std::string RedisDataSource::key_for_kind(
    integrations::ISerializedItemKind const& kind) const {
    return prefix_ + ":" + kind.Namespace();
}

RedisDataSource::RedisDataSource(std::unique_ptr<sw::redis::Redis> redis,
                                 std::string prefix)
    : prefix_(std::move(prefix)),
      inited_key_(prefix_ + ":$inited"),
      redis_(std::move(redis)) {}

RedisDataSource::~RedisDataSource() = default;

data_interfaces::ISerializedDataReader::GetResult RedisDataSource::Get(
    integrations::ISerializedItemKind const& kind,
    std::string const& itemKey) const {
    try {
        if (auto maybe_item = redis_->hget(key_for_kind(kind), itemKey)) {
            return integrations::SerializedItemDescriptor{0, false,
                                                          maybe_item.value()};
        }
        return tl::make_unexpected(Error{"not found"});
    } catch (sw::redis::Error const& e) {
        return tl::make_unexpected(Error{e.what()});
    }
}

data_interfaces::ISerializedDataReader::AllResult RedisDataSource::All(
    integrations::ISerializedItemKind const& kind) const {
    std::unordered_map<std::string, std::string> raw_items;
    AllResult::value_type items;

    try {
        redis_->hgetall(key_for_kind(kind),
                        std::inserter(raw_items, raw_items.begin()));
        for (auto const& [key, val] : raw_items) {
            items.emplace(
                key, integrations::SerializedItemDescriptor{0, false, val});
        }
        return items;

    } catch (sw::redis::Error const& e) {
        return tl::make_unexpected(Error{e.what()});
    }
}

std::string const& RedisDataSource::Identity() const {
    static std::string const identity = "redis";
    return identity;
}

bool RedisDataSource::Initialized() const {
    try {
        return redis_->exists(inited_key_) == 1;
    } catch (sw::redis::Error const& e) {
        return false;
    }
}

}  // namespace launchdarkly::server_side::data_systems
