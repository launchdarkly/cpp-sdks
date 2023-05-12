#include <launchdarkly/config/detail/builders/config_builder.hpp>
#include <launchdarkly/config/detail/defaults.hpp>
#include <launchdarkly/console_backend.hpp>

namespace launchdarkly::config::detail::builders {

template <typename SDK>
ConfigBuilder<SDK>::ConfigBuilder(std::string sdk_key)
    : sdk_key_(std::move(sdk_key)) {}

template <typename SDK>
ConfigBuilder<SDK>& ConfigBuilder<SDK>::ServiceEndpoints(
    EndpointsBuilder builder) {
    service_endpoints_builder_ = std::move(builder);
    return *this;
}

template <typename SDK>
ConfigBuilder<SDK>& ConfigBuilder<SDK>::Events(EventsBuilder builder) {
    events_builder_ = std::move(builder);
    return *this;
}

template <typename SDK>
ConfigBuilder<SDK>& ConfigBuilder<SDK>::AppInfo(AppInfoBuilder builder) {
    app_info_builder_ = std::move(builder);
    return *this;
}

template <typename SDK>
ConfigBuilder<SDK>& ConfigBuilder<SDK>::Offline(bool offline) {
    offline_ = offline;
    return *this;
}

template <typename SDK>
ConfigBuilder<SDK>& ConfigBuilder<SDK>::DataSource(DataSourceBuilder builder) {
    data_source_builder_ = builder;
    return *this;
}

template <typename SDK>
ConfigBuilder<SDK>& ConfigBuilder<SDK>::HttpProperties(
    HttpPropertiesBuilder builder) {
    http_properties_builder_ = builder;
    return *this;
}

template <typename SDK>
[[nodiscard]] tl::expected<typename ConfigBuilder<SDK>::Result, Error>
ConfigBuilder<SDK>::Build() const {
    auto sdk_key = sdk_key_;
    if (sdk_key.empty()) {
        return tl::make_unexpected(Error::kConfig_SDKKey_Empty);
    }
    auto offline = offline_.value_or(Defaults<SDK>::Offline());
    auto endpoints_config =
        service_endpoints_builder_.value_or(EndpointsBuilder()).Build();
    if (!endpoints_config) {
        return tl::make_unexpected(endpoints_config.error());
    }
    auto events_config = events_builder_.value_or(EventsBuilder()).Build();
    if (!events_config) {
        return tl::make_unexpected(events_config.error());
    }

    Logger logger{std::make_unique<ConsoleBackend>("LaunchDarkly")};

    std::optional<std::string> app_tag;
    if (app_info_builder_) {
        app_tag = app_info_builder_->Build(logger);
    }

    auto data_source_config = data_source_builder_
                                  ? data_source_builder_.value().Build()
                                  : Defaults<SDK>::DataSourceConfig();

    auto http_properties = http_properties_builder_
                               ? http_properties_builder_.value().Build()
                               : Defaults<SDK>::HttpProperties();

    return {tl::in_place,
            sdk_key,
            offline,
            std::move(logger),
            *endpoints_config,
            *events_config,
            app_tag,
            std::move(data_source_config),
            std::move(http_properties)};
}

template class ConfigBuilder<detail::ClientSDK>;
template class ConfigBuilder<detail::ServerSDK>;

}  // namespace launchdarkly::config::detail::builders
