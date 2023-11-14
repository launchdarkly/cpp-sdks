#include <launchdarkly/config/shared/builders/data_system/data_system_builder.hpp>

namespace launchdarkly::config::shared::builders {

DataSystemBuilder<ServerSDK>::DataSystemBuilder()
    : config_(Defaults<ServerSDK>::DataSystemConfig()) {}

DataSystemBuilder<ServerSDK>& DataSystemBuilder<ServerSDK>::Method(
    BackgroundSync bg_sync) {
    config_.system_ = bg_sync.Build();
    return *this;
}

DataSystemBuilder<ServerSDK>& DataSystemBuilder<ServerSDK>::Disabled(
    bool const disabled) {
    config_.disabled = disabled;
    return *this;
}

built::DataSystemConfig<ServerSDK> DataSystemBuilder<ServerSDK>::Build() const {
    return config_;
}

}  // namespace launchdarkly::config::shared::builders
