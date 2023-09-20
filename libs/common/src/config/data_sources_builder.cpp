
#include <launchdarkly/config/shared/builders/data_sources_builder.hpp>

namespace launchdarkly::config::shared::builders {

BootstrapBuilder::BootstrapBuilder()
    : order_(Order::ConsistentFirst), seed_(std::nullopt) {}

BootstrapBuilder& BootstrapBuilder::Order(enum BootstrapBuilder::Order order) {
    order_ = order;
    return *this;
}

BootstrapBuilder& BootstrapBuilder::RandomSeed(
    BootstrapBuilder::SeedType seed) {
    seed_ = seed;
    return *this;
}

DataSourcesBuilder<ServerSDK>::DataSourcesBuilder()
    : bootstrap_(), sources_() {}

DataSourceBuilder<ServerSDK>& DataSourcesBuilder<ServerSDK>::Source() {
    return sources_.emplace_back();
}

DataSourcesBuilder<ServerSDK>& DataSourcesBuilder<ServerSDK>::Destination() {
    return *this;
}

BootstrapBuilder& DataSourcesBuilder<ServerSDK>::Bootstrap() {
    return bootstrap_;
}

}  // namespace launchdarkly::config::shared::builders
