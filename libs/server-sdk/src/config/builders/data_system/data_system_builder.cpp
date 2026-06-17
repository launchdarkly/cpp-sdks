#include <launchdarkly/server_side/config/builders/data_system/data_system_builder.hpp>
#include "defaults.hpp"

namespace launchdarkly::server_side::config::builders {

DataSystemBuilder::DataSystemBuilder()
    : method_builder_(std::nullopt), config_(Defaults::DataSystemConfig()) {}

DataSystemBuilder& DataSystemBuilder::Method(BackgroundSync bg_sync) {
    method_builder_ = std::move(bg_sync);
    return *this;
}

DataSystemBuilder& DataSystemBuilder::Method(LazyLoad lazy_load) {
    method_builder_ = std::move(lazy_load);
    return *this;
}

DataSystemBuilder& DataSystemBuilder::Method(FDv2 fdv2) {
    method_builder_ = std::move(fdv2);
    return *this;
}

DataSystemBuilder& DataSystemBuilder::Enabled(bool const enabled) {
    config_.disabled = !enabled;
    return *this;
}

DataSystemBuilder& DataSystemBuilder::Disable() {
    return Enabled(false);
}

tl::expected<built::DataSystemConfig, Error> DataSystemBuilder::Build() const {
    if (method_builder_) {
        auto system_cfg = std::visit(
            [](auto&& arg)
                -> tl::expected<std::variant<built::LazyLoadConfig,
                                             built::BackgroundSyncConfig,
                                             built::FDv2Config>,
                                Error> {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, BackgroundSync>) {
                    return arg.Build();  // -> built::BackgroundSyncConfig
                } else if constexpr (std::is_same_v<T, LazyLoad>) {
                    return arg
                        .Build();  // -> tl::expected<built::LazyLoadConfig,
                                   // Error>
                } else if constexpr (std::is_same_v<T, FDv2>) {
                    return arg.Build();  // -> built::FDv2Config
                }
            },
            *method_builder_);
        if (!system_cfg) {
            return tl::make_unexpected(system_cfg.error());
        }
        return built::DataSystemConfig{config_.disabled,
                                       std::move(*system_cfg)};
    }
    return config_;
}

}  // namespace launchdarkly::server_side::config::builders
