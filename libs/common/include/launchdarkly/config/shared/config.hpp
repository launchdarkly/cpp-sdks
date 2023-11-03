#pragma once

#include <launchdarkly/config/shared/builders/endpoints_builder.hpp>
#include <launchdarkly/config/shared/builders/events_builder.hpp>
#include <launchdarkly/config/shared/built/data_source_config.hpp>
#include <launchdarkly/config/shared/built/data_system/data_system_config.hpp>
#include <launchdarkly/config/shared/built/events.hpp>
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <launchdarkly/config/shared/built/logging.hpp>
#include <launchdarkly/config/shared/built/persistence.hpp>
#include <launchdarkly/config/shared/built/service_endpoints.hpp>
#include <launchdarkly/config/shared/sdks.hpp>

namespace launchdarkly::config::shared {

/**
 * Config represents the configuration for a LaunchDarkly C++ SDK.
 * It should be passed into the SDK's constructor.
 * @tparam SDK Type of SDK.
 */

template <typename SDK>
struct Config {};
template <>
struct Config<shared::ClientSDK> {
   public:
    using SDK = shared::ClientSDK;
    Config(std::string sdk_key,
           bool offline,
           shared::built::Logging logging,
           shared::built::ServiceEndpoints endpoints,
           shared::built::Events events,
           std::optional<std::string> application_tag,
           shared::built::DataSourceConfig<SDK> data_source_config,
           shared::built::HttpProperties http_properties,
           shared::built::Persistence<SDK> persistence);

    [[nodiscard]] std::string const& SdkKey() const;

    [[nodiscard]] shared::built::ServiceEndpoints const& ServiceEndpoints()
        const;

    [[nodiscard]] shared::built::Events const& Events() const;

    [[nodiscard]] std::optional<std::string> const& ApplicationTag() const;

    [[nodiscard]] config::shared::built::DataSourceConfig<SDK> const&
    DataSourceConfig() const;

    [[nodiscard]] shared::built::HttpProperties const& HttpProperties() const;

    [[nodiscard]] bool Offline() const;

    [[nodiscard]] shared::built::Logging const& Logging() const;

    [[nodiscard]] shared::built::Persistence<SDK> const& Persistence() const;

   private:
    std::string sdk_key_;
    bool offline_;
    config::shared::built::Logging logging_;
    config::shared::built::ServiceEndpoints service_endpoints_;
    std::optional<std::string> application_tag_;
    config::shared::built::Events events_;
    config::shared::built::DataSourceConfig<SDK> data_source_config_;
    config::shared::built::HttpProperties http_properties_;
    shared::built::Persistence<SDK> persistence_;
};

template <>
struct Config<shared::ServerSDK> {
   public:
    using SDK = shared::ServerSDK;
    Config(std::string sdk_key,
           bool offline,
           shared::built::Logging logging,
           shared::built::ServiceEndpoints endpoints,
           shared::built::Events events,
           std::optional<std::string> application_tag,
           shared::built::DataSystemConfig<SDK> data_system_config,
           shared::built::HttpProperties http_properties);

    [[nodiscard]] std::string const& SdkKey() const;

    [[nodiscard]] shared::built::ServiceEndpoints const& ServiceEndpoints()
        const;

    [[nodiscard]] shared::built::Events const& Events() const;

    [[nodiscard]] std::optional<std::string> const& ApplicationTag() const;

    [[nodiscard]] config::shared::built::DataSystemConfig<SDK> const&
    DataSystemConfig() const;

    [[nodiscard]] shared::built::HttpProperties const& HttpProperties() const;

    [[nodiscard]] bool Offline() const;

    [[nodiscard]] shared::built::Logging const& Logging() const;

    [[nodiscard]] shared::built::Persistence<SDK> const& Persistence() const;

   private:
    std::string sdk_key_;
    bool offline_;
    config::shared::built::Logging logging_;
    config::shared::built::ServiceEndpoints service_endpoints_;
    std::optional<std::string> application_tag_;
    config::shared::built::Events events_;
    shared::built::DataSystemConfig<SDK> data_system_config_;
    config::shared::built::HttpProperties http_properties_;
};

}  // namespace launchdarkly::config::shared
