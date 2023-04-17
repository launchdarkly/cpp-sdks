#pragma once

#include "config/detail/service_endpoints.hpp"
#include "error.hpp"

#include <tl/expected.hpp>

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
 * EndpointsBuilder allows for specification of LaunchDarkly service Endpoints.
 *
 * @tparam SDK Type of SDK, such as ClientSDK or ServerSDK.
 */
template <typename SDK>
class EndpointsBuilder {
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
     * returns an error.
     * @return Unique pointer to ServiceEndpoints, or error.
     */
    [[nodiscard]] tl::expected<ServiceEndpoints, Error> build();

   private:
    std::optional<std::string> polling_base_url_;
    std::optional<std::string> streaming_base_url_;
    std::optional<std::string> events_base_url_;
};

}  // namespace launchdarkly::config::detail
