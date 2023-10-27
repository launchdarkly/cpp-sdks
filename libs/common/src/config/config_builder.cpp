#include <launchdarkly/config/shared/builders/config_builder.hpp>
#include <launchdarkly/config/shared/defaults.hpp>

namespace launchdarkly::config::shared::builders {

ConfigBuilder<ClientSDK>::ConfigBuilder(std::string sdk_key)
    : sdk_key_(std::move(sdk_key)) {}

ConfigBuilder<ServerSDK>::ConfigBuilder(std::string sdk_key)
    : sdk_key_(std::move(sdk_key)) {}

typename ConfigBuilder<ClientSDK>::EndpointsBuilder&
ConfigBuilder<ClientSDK>::ServiceEndpoints() {
    return service_endpoints_builder_;
}

typename ConfigBuilder<ServerSDK>::EndpointsBuilder&
ConfigBuilder<ServerSDK>::ServiceEndpoints() {
    return service_endpoints_builder_;
}

typename ConfigBuilder<ClientSDK>::EventsBuilder&
ConfigBuilder<ClientSDK>::Events() {
    return events_builder_;
}

typename ConfigBuilder<ServerSDK>::EventsBuilder&
ConfigBuilder<ServerSDK>::Events() {
    return events_builder_;
}

AppInfoBuilder& ConfigBuilder<ClientSDK>::AppInfo() {
    return app_info_builder_;
}

AppInfoBuilder& ConfigBuilder<ServerSDK>::AppInfo() {
    return app_info_builder_;
}

ConfigBuilder<ClientSDK>& ConfigBuilder<ClientSDK>::Offline(bool offline) {
    offline_ = offline;
    return *this;
}

typename ConfigBuilder<ClientSDK>::DataSourceBuilder&
ConfigBuilder<ClientSDK>::DataSource() {
    return data_source_builder_;
}

typename ConfigBuilder<ServerSDK>::DataSystemBuilder&
ConfigBuilder<ServerSDK>::DataSystem() {
    return data_system_builder_;
}

typename ConfigBuilder<ClientSDK>::HttpPropertiesBuilder&
ConfigBuilder<ClientSDK>::HttpProperties() {
    return http_properties_builder_;
}

typename ConfigBuilder<ServerSDK>::HttpPropertiesBuilder&
ConfigBuilder<ServerSDK>::HttpProperties() {
    return http_properties_builder_;
}

LoggingBuilder& ConfigBuilder<ClientSDK>::Logging() {
    return logging_config_builder_;
}

LoggingBuilder& ConfigBuilder<ServerSDK>::Logging() {
    return logging_config_builder_;
}

PersistenceBuilder<ClientSDK>& ConfigBuilder<ClientSDK>::Persistence() {
    return persistence_builder_;
}

[[nodiscard]] tl::expected<typename ConfigBuilder<ClientSDK>::Result, Error>
ConfigBuilder<ClientSDK>::Build() const {
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

[[nodiscard]] tl::expected<typename ConfigBuilder<ServerSDK>::Result, Error>
ConfigBuilder<ServerSDK>::Build() const {
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

    auto data_system_config = data_system_builder_.Build();

    auto http_properties = http_properties_builder_.Build();

    auto logging = logging_config_builder_.Build();

    return {tl::in_place,
            sdk_key,
            offline,
            logging,
            *endpoints_config,
            *events_config,
            app_tag,
            std::move(data_system_config),
            std::move(http_properties)};
}

}  // namespace launchdarkly::config::shared::builders
