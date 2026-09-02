#include <launchdarkly/config/shared/builders/config_builder.hpp>
#include <launchdarkly/config/shared/defaults.hpp>

#include <type_traits>
#include <variant>

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

    if constexpr (std::is_same_v<SDK, ClientSDK>) {
        // FDv2 polls a different endpoint than FDv1 does, so it keeps its own
        // default until the application configures its own endpoints. Once it
        // has, FDv2 follows them, so that pointing the SDK at a Relay Proxy
        // redirects FDv2 too.
        if (auto* fdv2 =
                std::get_if<built::FDv2Config<SDK>>(&data_source_config.method);
            fdv2 != nullptr && service_endpoints_builder_.IsCustom()) {
            fdv2->polling_base_url = endpoints_config->PollingBaseUrl();
            fdv2->streaming_base_url = endpoints_config->StreamingBaseUrl();
        }
    }

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

}  // namespace launchdarkly::config::shared::builders
