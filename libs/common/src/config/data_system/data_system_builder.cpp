#include <launchdarkly/config/shared/builders/data_system/data_system_builder.hpp>

namespace launchdarkly::config::shared::builders {

DataSystemBuilder<ServerSDK>::DataSystemBuilder()
    : method_builder_(std::nullopt),
      config_(Defaults<ServerSDK>::DataSystemConfig()) {}

DataSystemBuilder<ServerSDK>& DataSystemBuilder<ServerSDK>::Method(
    BackgroundSync bg_sync) {
    method_builder_ = std::move(bg_sync);
    return *this;
}

DataSystemBuilder<ServerSDK>& DataSystemBuilder<ServerSDK>::Method(
    LazyLoad lazy_load) {
    method_builder_ = std::move(lazy_load);
    return *this;
}

DataSystemBuilder<ServerSDK>& DataSystemBuilder<ServerSDK>::Disabled(
    bool const disabled) {
    config_.disabled = disabled;
    return *this;
}

tl::expected<built::DataSystemConfig<ServerSDK>, Error>
DataSystemBuilder<ServerSDK>::Build() const {
    if (method_builder_) {
        auto lazy_or_background_cfg = std::visit(
            [](auto&& arg)
                -> tl::expected<
                    std::variant<built::LazyLoadConfig,
                                 built::BackgroundSyncConfig<ServerSDK>>,
                    Error> {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, BackgroundSync>) {
                    return arg.Build();  // -> built::BackgroundSyncConfig
                } else if constexpr (std::is_same_v<T, LazyLoad>) {
                    return arg
                        .Build();  // -> tl::expected<built::LazyLoadConfig,
                                   // Error>
                }
            },
            *method_builder_);
        if (!lazy_or_background_cfg) {
            return tl::make_unexpected(lazy_or_background_cfg.error());
        }
        return built::DataSystemConfig<ServerSDK>{
            config_.disabled, std::move(*lazy_or_background_cfg)};
    }
    return config_;
}

}  // namespace launchdarkly::config::shared::builders
