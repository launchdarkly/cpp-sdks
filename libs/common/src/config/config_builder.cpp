#include "config/detail/config_builder.hpp"
#include "config/detail/defaults.hpp"
#include "console_backend.hpp"

namespace launchdarkly::config::detail {
template <typename SDK>
ConfigBuilder<SDK>::ConfigBuilder(std::string sdk_key)
    : sdk_key_(std::move(sdk_key)) {}

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
[[nodiscard]] typename ConfigBuilder<SDK>::ConfigResult
ConfigBuilder<SDK>::build() const {
    auto sdk_key = sdk_key_;
    if (sdk_key.empty()) {
        return tl::make_unexpected(Error::kConfig_SDKKey_Empty);
    }
    auto offline = offline_.value_or(Defaults<SDK>::offline());
    auto endpoints_config =
        service_endpoints_builder_.value_or(EndpointsBuilder()).build();
    if (!endpoints_config) {
        return tl::make_unexpected(endpoints_config.error());
    }
    auto events_config = events_builder_.value_or(EventsBuilder()).build();
    if (!events_config) {
        return tl::make_unexpected(events_config.error());
    }

    Logger logger{std::make_unique<ConsoleBackend>("LaunchDarkly")};

    std::optional<std::string> app_tag;
    if (application_info_builder_) {
        app_tag = application_info_builder_->build(logger);
    }

    auto data_source_config = data_source_builder_
                                  ? data_source_builder_.value().build()
                                  : Defaults<SDK>::data_source_config();

    auto http_properties = http_properties_builder_
                               ? http_properties_builder_.value().build()
                               : Defaults<SDK>::http_properties();

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

}  // namespace launchdarkly::config::detail
