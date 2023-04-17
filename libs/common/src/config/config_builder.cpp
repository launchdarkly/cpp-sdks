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
ConfigBuilder<SDK>& ConfigBuilder<SDK>::events(
    detail::EventsBuilder<SDK> builder) {
    events_builder_ = std::move(builder);
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
ConfigBuilder<SDK>& ConfigBuilder<SDK>::http_properties(
    detail::HttpPropertiesBuilder<SDK> builder) {
    http_properties_builder_ = builder;
    return *this;
}

template <typename SDK>
typename ConfigBuilder<SDK>::ConfigType ConfigBuilder<SDK>::build(
    Logger& logger) const {
    auto key = sdk_key_;
    auto offline = offline_.value_or(Defaults<detail::AnySDK>::offline());
    auto endpoints = service_endpoints_builder_.value_or(EndpointsBuilder());
    auto events = events_builder_.value_or(EventsBuilder());
    std::optional<std::string> app_tag;
    if (application_info_builder_) {
        app_tag = application_info_builder_->Build(logger);
    }
    auto data_source_config = data_source_builder_
                                  ? data_source_builder_.value().build()
                                  : Defaults<SDK>::data_source_config();

    auto http_properties = http_properties_builder_
                               ? http_properties_builder_.value().build()
                               : Defaults<SDK>::http_properties();
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
