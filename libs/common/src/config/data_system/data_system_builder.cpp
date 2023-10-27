#include <launchdarkly/config/shared/builders/data_system/data_systems_builder.hpp>

namespace launchdarkly::config::shared::builders {

DataSystemBuilder<ServerSDK>::DataSystemBuilder()
    : config_(Defaults<ServerSDK>::DataSystemConfig()) {}

DataSystemBuilder<ServerSDK>& DataSystemBuilder<ServerSDK>::BackgroundSync(
    BackgroundSyncBuilder builder) {
    config_.system_ = builder.Build();
    return *this;
}

built::DataSystemConfig<ServerSDK> DataSystemBuilder<ServerSDK>::Build() const {
    return config_;
}

}  // namespace launchdarkly::config::shared::builders
