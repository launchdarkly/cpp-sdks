#pragma once

#include "config/detail/service_hosts.hpp"
#include "error.hpp"

#include <tl/expected.hpp>

#include <memory>
#include <optional>
#include <string>

namespace launchdarkly::config::detail {

template <typename SDK>
class HostsBuilder;

template <typename SDK>
bool operator==(HostsBuilder<SDK> const& lhs, HostsBuilder<SDK> const& rhs);

/**
 * HostsBuilder allows for specification of LaunchDarkly service hosts.
 *
 * @tparam SDK Type of SDK, such as ClientSDK or ServerSDK.
 */
template <typename SDK>
class HostsBuilder {
   public:
    friend bool operator==
        <SDK>(HostsBuilder<SDK> const& lhs, HostsBuilder<SDK> const& rhs);
    /**
     * Constructs an HostsBuilder.
     */
    HostsBuilder() = default;

    /**
     * Sets a custom host for the polling service.
     * @param host Host to set.
     * @return Reference to this builder.
     */
    HostsBuilder& polling_host(std::string host);

    /**
     * Sets a custom host for the streaming service.
     * @param host host to set.
     * @return Reference to this builder.
     */
    HostsBuilder& streaming_host(std::string host);

    /**
     * Sets a custom host for the events service.
     * @param host host to set.
     * @return Reference to this builder.
     */
    HostsBuilder& events_host(std::string host);

    /**
     * Sets a host for a Relay Proxy instance, which is equivalent to specifying
     * this host for streaming, polling, and events.
     * @param host host to set.
     * @return Reference to this builder.
     */
    HostsBuilder& relay_proxy(std::string const& host);

    /**
     * Builds a ServiceHosts if the configuration is valid. If not,
     * returns an error.
     * @return ServiceHosts, or error.
     */
    [[nodiscard]] tl::expected<ServiceHosts, Error> build();

   private:
    std::optional<std::string> polling_host_;
    std::optional<std::string> streaming_host_;
    std::optional<std::string> events_host_;
};

}  // namespace launchdarkly::config::detail
