#include <launchdarkly/config/shared/builders/config_builder.hpp>
#include <launchdarkly/config/shared/defaults.hpp>

namespace launchdarkly::config::shared::builders {

template <typename SDK>
ConfigBuilder<SDK>::ConfigBuilder(std::string sdk_key)
    : sdk_key_(std::move(sdk_key)) {}

template <typename SDK>
typename ConfigBuilder<SDK>::EndpointsBuilder&
ConfigBuilder<SDK>::ServiceEndpoints() {
    return service_endpoints_builder_;
}

template <typename SDK>
typename ConfigBuilder<SDK>::EventsBuilder& ConfigBuilder<SDK>::Events() {
    return events_builder_;
}

template <typename SDK>
AppInfoBuilder& ConfigBuilder<SDK>::AppInfo() {
    return app_info_builder_;
}

template <typename SDK>
ConfigBuilder<SDK>& ConfigBuilder<SDK>::Offline(bool offline) {
    offline_ = offline;
    return *this;
}

template <typename SDK>
typename ConfigBuilder<SDK>::DataSourceBuilder&
ConfigBuilder<SDK>::DataSource() {
    return data_source_builder_;
}

template <typename SDK>
typename ConfigBuilder<SDK>::HttpPropertiesBuilder&
ConfigBuilder<SDK>::HttpProperties() {
    return http_properties_builder_;
}

template <typename SDK>
LoggingBuilder& ConfigBuilder<SDK>::Logging() {
    return logging_config_builder_;
}

template <typename SDK>
PersistenceBuilder<SDK>& ConfigBuilder<SDK>::Persistence() {
    return persistence_builder_;
}

template <typename SDK>
[[nodiscard]] tl::expected<typename ConfigBuilder<SDK>::Result, Error>
ConfigBuilder<SDK>::Build() const {
    auto sdk_key = sdk_key_;
    if (sdk_key.empty()) {
        return tl::make_unexpected(Error::kConfig_SDKKey_Empty);
    }
    auto offline = offline_.value_or(Defaults<SDK>::Offline());
    auto endpoints_config = service_endpoints_builder_.Build();
    if (!endpoints_config) {
        return tl::make_unexpected(endpoints_config.error());
    }
    auto events_config = events_builder_.Build();
    if (!events_config) {
        return tl::make_unexpected(events_config.error());
    }

    std::optional<std::string> app_tag = app_info_builder_.Build();

    auto data_source_config = data_source_builder_.Build();

    auto http_properties = http_properties_builder_.Build();

    auto logging = logging_config_builder_.Build();

    auto persistence = persistence_builder_.Build();

    return {tl::in_place,
            sdk_key,
            offline,
            logging,
            *endpoints_config,
            *events_config,
            app_tag,
            std::move(data_source_config),
            std::move(http_properties),
            std::move(persistence)};
}

template class ConfigBuilder<config::shared::ClientSDK>;
template class ConfigBuilder<config::shared::ServerSDK>;

}  // namespace launchdarkly::config::shared::builders
