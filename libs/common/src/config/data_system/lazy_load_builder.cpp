#include <launchdarkly/config/shared/builders/data_system/lazy_load_builder.hpp>

namespace launchdarkly::config::shared::builders {

LazyLoadBuilder::LazyLoadBuilder()
    : redis_builder_(), config_(Defaults<ServerSDK>::LazyLoadConfig()) {}

LazyLoadBuilder& LazyLoadBuilder::CacheTTL(
    std::chrono::milliseconds const ttl) {
    config_.eviction_ttl = ttl;
    return *this;
}

LazyLoadBuilder& LazyLoadBuilder::CacheEviction(EvictionPolicy const policy) {
    config_.eviction_policy = policy;
    return *this;
}

LazyLoadBuilder& LazyLoadBuilder::Source(Redis source) {
    redis_builder_ = std::move(source);
    return *this;
}

tl::expected<built::LazyLoadConfig, Error> LazyLoadBuilder::Build() const {
    auto redis_config = redis_builder_.Build();
    if (!redis_config) {
        return tl::make_unexpected(redis_config.error());
    }
    auto copy = config_;
    copy.source = {*redis_config};
    return copy;
}

}  // namespace launchdarkly::config::shared::builders
