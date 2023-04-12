#include "config/detail/config_builder.hpp"
#include "config/detail/defaults.hpp"

namespace launchdarkly::config::detail {
template <typename SDK>
ConfigBuilder<SDK>::ConfigBuilder(std::string sdk_key)
    : sdk_key_(std::move(sdk_key)),
      offline_(std::nullopt),
      service_endpoints_builder_(std::nullopt) {}

template <typename SDK>
ConfigBuilder<SDK>& ConfigBuilder<SDK>::service_endpoints(
    detail::EndpointsBuilder<SDK> builder) {
    service_endpoints_builder_ = std::move(builder);
    return *this;
}

template <typename SDK>
ConfigBuilder<SDK>& ConfigBuilder<SDK>::application_info(
    detail::ApplicationInfo builder) {
    application_info_builder_ = std::move(builder);
    return *this;
}

template <typename SDK>
ConfigBuilder<SDK>& ConfigBuilder<SDK>::offline(bool offline) {
    offline_ = offline;
    return *this;
}

template <typename SDK>
ConfigBuilder<SDK>& ConfigBuilder<SDK>::data_source(
    detail::DataSourceBuilder<SDK> builder) {
    data_source_builder_ = builder;
    return *this;
}

template <typename SDK>
typename ConfigBuilder<SDK>::ConfigType ConfigBuilder<SDK>::build(
    Logger& logger) const {
    auto key = sdk_key_;
    auto offline = offline_.value_or(Defaults<detail::AnySDK>::offline());
    auto endpoints = service_endpoints_builder_.value_or(
        ConfigBuilder<SDK>::EndpointsBuilder());
    std::optional<std::string> app_tag;
    if (application_info_builder_) {
        app_tag = application_info_builder_->build(logger);
    }
    auto data_source_config = data_source_builder_
                                  ? data_source_builder_.value().build()
                                  : Defaults<SDK>::data_source_config();
    return {std::move(key), offline, std::move(endpoints), std::move(app_tag),
            std::move(data_source_config)};
}

template class ConfigBuilder<detail::ClientSDK>;
template class ConfigBuilder<detail::ServerSDK>;

}  // namespace launchdarkly::config::detail
