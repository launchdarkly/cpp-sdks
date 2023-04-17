#include "config/detail/config_builder.hpp"
#include "config/detail/defaults.hpp"

namespace launchdarkly::config::detail {
template <typename SDK>
ConfigBuilder<SDK>::ConfigBuilder(std::string sdk_key)
    : sdk_key_(std::move(sdk_key)),
      offline_(std::nullopt),
      service_endpoints_builder_(std::nullopt) {}

template <typename SDK>
ConfigBuilder<SDK>& ConfigBuilder<SDK>::ServiceEndpoints(
    detail::EndpointsBuilder<SDK> builder) {
    service_endpoints_builder_ = std::move(builder);
    return *this;
}

template <typename SDK>
ConfigBuilder<SDK>& ConfigBuilder<SDK>::Events(
    detail::EventsBuilder<SDK> builder) {
    events_builder_ = std::move(builder);
    return *this;
}

template <typename SDK>
ConfigBuilder<SDK>& ConfigBuilder<SDK>::AppInfo(
    detail::AppInfoBuilder builder) {
    app_info_builder_ = std::move(builder);
    return *this;
}

template <typename SDK>
ConfigBuilder<SDK>& ConfigBuilder<SDK>::Offline(bool offline) {
    offline_ = offline;
    return *this;
}

template <typename SDK>
ConfigBuilder<SDK>& ConfigBuilder<SDK>::DataSource(
    detail::DataSourceBuilder<SDK> builder) {
    data_source_builder_ = builder;
    return *this;
}

template <typename SDK>
ConfigBuilder<SDK>& ConfigBuilder<SDK>::HttpProperties(
    detail::HttpPropertiesBuilder<SDK> builder) {
    http_properties_builder_ = builder;
    return *this;
}

template <typename SDK>
typename ConfigBuilder<SDK>::ConfigType ConfigBuilder<SDK>::Build(
    Logger& logger) const {
    auto key = sdk_key_;
    auto offline = offline_.value_or(Defaults<detail::AnySDK>::Offline());
    auto endpoints = service_endpoints_builder_.value_or(EndpointsBuilder());
    auto events = events_builder_.value_or(EventsBuilder());
    std::optional<std::string> app_tag;
    if (app_info_builder_) {
        app_tag = app_info_builder_->Build(logger);
    }
    auto data_source_config = data_source_builder_
                                  ? data_source_builder_.value().build()
                                  : Defaults<SDK>::DataSource();

    auto http_properties = http_properties_builder_
                               ? http_properties_builder_.value().build()
                               : Defaults<SDK>::HttpProperties();
    return {std::move(key),
            offline,
            std::move(endpoints),
            std::move(events),
            std::move(app_tag),
            std::move(data_source_config),
            std::move(http_properties)};
}

template class ConfigBuilder<detail::ClientSDK>;
template class ConfigBuilder<detail::ServerSDK>;

}  // namespace launchdarkly::config::detail
