#include <launchdarkly/server_side/config/config_builder.hpp>

namespace launchdarkly::server_side {

ConfigBuilder::ConfigBuilder(std::string sdk_key)
    : sdk_key_(std::move(sdk_key)), offline_(false) {}

config::builders::EndpointsBuilder& ConfigBuilder::ServiceEndpoints() {
    return service_endpoints_builder_;
}

config::builders::EventsBuilder& ConfigBuilder::Events() {
    return events_builder_;
}

config::builders::AppInfoBuilder& ConfigBuilder::AppInfo() {
    return app_info_builder_;
}

config::builders::DataSystemBuilder& ConfigBuilder::DataSystem() {
    return data_system_builder_;
}

config::builders::HttpPropertiesBuilder& ConfigBuilder::HttpProperties() {
    return http_properties_builder_;
}

config::builders::LoggingBuilder& ConfigBuilder::Logging() {
    return logging_config_builder_;
}

ConfigBuilder& ConfigBuilder::Offline(bool const offline) {
    offline_ = offline;
    return *this;
}

tl::expected<Config, Error> ConfigBuilder::Build() const {
    auto sdk_key = sdk_key_;
    if (sdk_key.empty()) {
        return tl::make_unexpected(Error::kConfig_SDKKey_Empty);
    }

    auto endpoints_config = service_endpoints_builder_.Build();
    if (!endpoints_config) {
        return tl::make_unexpected(endpoints_config.error());
    }

    auto events_builder = events_builder_;
    if (offline_) {
        events_builder.Disable();
    }
    auto events_config = events_builder.Build();
    if (!events_config) {
        return tl::make_unexpected(events_config.error());
    }

    std::optional<std::string> app_tag = app_info_builder_.Build();

    auto data_system_builder = data_system_builder_;
    if (offline_) {
        data_system_builder.Disable();
    }
    auto data_system_config = data_system_builder.Build();
    if (!data_system_config) {
        return tl::make_unexpected(data_system_config.error());
    }

    auto http_properties = http_properties_builder_.Build();

    auto logging = logging_config_builder_.Build();

    return {tl::in_place,
            sdk_key,
            logging,
            *endpoints_config,
            *events_config,
            app_tag,
            std::move(*data_system_config),
            std::move(http_properties)};
}

}  // namespace launchdarkly::server_side
