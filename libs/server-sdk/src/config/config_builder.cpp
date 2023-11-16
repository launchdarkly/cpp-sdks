#include <launchdarkly/server_side/config/config_builder.hpp>
#include "launchdarkly/config/shared/defaults.hpp"

namespace launchdarkly::server_side {

ConfigBuilder::ConfigBuilder(std::string sdk_key)
    : sdk_key_(std::move(sdk_key)) {}

EndpointsBuilder& ConfigBuilder::ServiceEndpoints() {
    return service_endpoints_builder_;
}

EventsBuilder& ConfigBuilder::Events() {
    return events_builder_;
}

AppInfoBuilder& ConfigBuilder::AppInfo() {
    return app_info_builder_;
}

ConfigBuilder& ConfigBuilder::Offline(bool offline) {
    offline_ = offline;
    return *this;
}

DataSourceBuilder& ConfigBuilder::DataSource() {
    return data_source_builder_;
}

HttpPropertiesBuilder& ConfigBuilder::HttpProperties() {
    return http_properties_builder_;
}

LoggingBuilder& ConfigBuilder::Logging() {
    return logging_config_builder_;
}

tl::expected<Config, Error> ConfigBuilder::Build() const {
    auto sdk_key = sdk_key_;
    if (sdk_key.empty()) {
        return tl::make_unexpected(Error::kConfig_SDKKey_Empty);
    }
    auto offline = offline_.value_or(Defaults::Offline());
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

    return {tl::in_place,
            sdk_key,
            offline,
            logging,
            *endpoints_config,
            *events_config,
            app_tag,
            std::move(data_source_config),
            std::move(http_properties)};
}

}  // namespace launchdarkly::server_side
