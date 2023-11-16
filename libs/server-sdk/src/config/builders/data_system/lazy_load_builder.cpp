#include <launchdarkly/server_side/config/builders/data_system/lazy_load_builder.hpp>

#include "defaults.hpp"

namespace launchdarkly::server_side::config::builders {

LazyLoadBuilder::LazyLoadBuilder() : config_(Defaults::LazyLoadConfig()) {}

LazyLoadBuilder& LazyLoadBuilder::CacheTTL(
    std::chrono::milliseconds const ttl) {
    config_.eviction_ttl = ttl;
    return *this;
}

LazyLoadBuilder& LazyLoadBuilder::CacheEviction(EvictionPolicy const policy) {
    config_.eviction_policy = policy;
    return *this;
}

LazyLoadBuilder& LazyLoadBuilder::Source(SourcePtr source) {
    config_.source = source;
    return *this;
}

tl::expected<built::LazyLoadConfig, Error> LazyLoadBuilder::Build() const {
    if (!config_.source) {
        return tl::make_unexpected(
            Error::kConfig_DataSystem_LazyLoad_MissingSource);
    }
    return config_;
}

}  // namespace launchdarkly::server_side::config::builders
