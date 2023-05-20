#pragma once

#include <launchdarkly/config/shared/built/service_endpoints.hpp>
#include <launchdarkly/error.hpp>

#include "tl/expected.hpp"

#include <memory>
#include <optional>
#include <string>

namespace launchdarkly::config::shared::builders {

template <typename SDK>
class EndpointsBuilder;

template <typename SDK>
bool operator==(EndpointsBuilder<SDK> const& lhs,
                EndpointsBuilder<SDK> const& rhs);

/**
 * EndpointsBuilder allows for specification of LaunchDarkly service
 * ServiceEndpoints.
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
    EndpointsBuilder& PollingBaseUrl(std::string url);

    /**
     * Sets a custom URL for the streaming service.
     * @param url URL to set.
     * @return Reference to this builder.
     */
    EndpointsBuilder& StreamingBaseUrl(std::string url);

    /**
     * Sets a custom URL for the events service.
     * @param url URL to set.
     * @return Reference to this builder.
     */
    EndpointsBuilder& EventsBaseUrl(std::string url);

    /**
     * Sets a custom URL for a Relay Proxy instance. The streaming,
     * polling, and events URLs are set automatically.
     * @param url URL to set.
     * @return Reference to this builder.
     */
    EndpointsBuilder& RelayProxyBaseURL(std::string const& url);

    /**
     * Builds a ServiceEndpoints if the configuration is valid. If not,
     * returns an error. If any streaming, polling, or events URL is set,
     * then all URLs must be set.
     * @return Unique pointer to ServiceEndpoints, or error.
     */
    [[nodiscard]] tl::expected<built::ServiceEndpoints, Error> Build() const;

   private:
    std::optional<std::string> polling_base_url_;
    std::optional<std::string> streaming_base_url_;
    std::optional<std::string> events_base_url_;
};

}  // namespace launchdarkly::config::shared::builders
