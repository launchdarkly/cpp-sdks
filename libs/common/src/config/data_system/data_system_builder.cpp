#include <launchdarkly/config/shared/builders/data_system/data_system_builder.hpp>

namespace launchdarkly::config::shared::builders {

DataSystemBuilder<ServerSDK>::DataSystemBuilder()
    : method_builder_(std::nullopt),
      config_(Defaults<ServerSDK>::DataSystemConfig()) {}

DataSystemBuilder<ServerSDK>& DataSystemBuilder<ServerSDK>::Method(
    BackgroundSync bg_sync) {
    method_config_ = bg_sync.Build();
    return *this;
}

DataSystemBuilder<ServerSDK>& DataSystemBuilder<ServerSDK>::Method(
    LazyLoad lazy_load) {
    method_config_ = lazy_load.Build();
    return *this;
}

DataSystemBuilder<ServerSDK>& DataSystemBuilder<ServerSDK>::Disabled(
    bool const disabled) {
    config_.disabled = disabled;
    return *this;
}

tl::expected<built::DataSystemConfig<ServerSDK>, Error>
DataSystemBuilder<ServerSDK>::Build() const {
    // We could also store the builders and do a std::visit here. Instead,
    // we're building immediately in the Method setters to reduce the visitor
    // boilerplate.

    if (method_config_) {
        auto maybe_built = *method_config_;
        if (!maybe_built) {
            return tl::make_unexpected(maybe_built.error());
        }
        return built::DataSystemConfig<ServerSDK>{config_.disabled,
                                                  *maybe_built};
    }

    return config_;
}

}  // namespace launchdarkly::config::shared::builders
