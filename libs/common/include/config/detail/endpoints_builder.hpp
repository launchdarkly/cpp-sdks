#pragma once

#include "service_endpoints.hpp"

#include <memory>
#include <optional>
#include <string>

namespace launchdarkly::config::detail {

template <typename SDK>
class EndpointsBuilder;

template <typename SDK>
bool operator==(EndpointsBuilder<SDK> const& lhs,
                EndpointsBuilder<SDK> const& rhs);

/**
 * EndpointsBuilder allows for specification of LaunchDarkly service endpoints.
 *
 * @tparam SDK Type of SDK, such as ClientSDK or ServerSDK.
 */
template <typename SDK>
class EndpointsBuilder {
   private:
    std::optional<std::string> polling_base_url_;
    std::optional<std::string> streaming_base_url_;
    std::optional<std::string> events_base_url_;

   public:
    friend bool operator==<SDK>(EndpointsBuilder<SDK> const& lhs,
                                EndpointsBuilder<SDK> const& rhs);
    /**
     * Constructs an EndpointsBuilder.
     */
    EndpointsBuilder() = default;

    /**
     * Sets a custom URL for the polling service.
     * @param url URL to set.
     * @return Reference to this builder.
     */
    EndpointsBuilder& polling_base_url(std::string url);

    /**
     * Sets a custom URL for the streaming service.
     * @param url URL to set.
     * @return Reference to this builder.
     */
    EndpointsBuilder& streaming_base_url(std::string url);

    /**
     * Sets a custom URL for the events service.
     * @param url URL to set.
     * @return Reference to this builder.
     */
    EndpointsBuilder& events_base_url(std::string url);

    /**
     * Sets a single base URL for a Relay Proxy instance.
     * @param url URL to set.
     * @return Reference to this builder.
     */
    EndpointsBuilder& relay_proxy(std::string const& url);

    /**
     * Builds a ServiceEndpoints if the configuration is valid. If not,
     * returns nullptr.
     * @return Unique pointer to ServiceEndpoints, or nullptr.
     */
    [[nodiscard]] std::unique_ptr<ServiceEndpoints> build();
};

}  // namespace launchdarkly::config::detail
