#pragma once

#include <string>

namespace launchdarkly::config {

/**
 * ServiceEndpoints contains configured Endpoints for the LaunchDarkly
 * service or a Relay Proxy instance.
 */
class ServiceEndpoints {
   public:
    /**
     * Constructs a ServiceEndpoints from individual polling, streaming, and
     * events URLs.
     *
     * Meant for internal usage only; see ClientEndpointsBuilder or
     * ServerEndpointsBuilder to safely construct a ServiceEndpoints with
     * default URLs.
     *
     * @param polling Polling URL.
     * @param streaming Streaming URL.
     * @param events Events URL.
     */
    ServiceEndpoints(std::string polling,
                     std::string streaming,
                     std::string events);
    /**
     * Returns the configured base polling URL.
     * @return Base polling URL.
     */
    [[nodiscard]] std::string const& PollingBaseUrl() const;
    /**
     * Returns the configured base streaming URL.
     * @return Base streaming URL.
     */
    [[nodiscard]] std::string const& StreamingBaseUrl() const;
    /**
     * Returns the configured base events URL.
     * @return Base events URL.
     */
    [[nodiscard]] std::string const& EventsBaseUrl() const;

   private:
    std::string polling_base_url_;
    std::string streaming_base_url_;
    std::string events_base_url_;
};

bool operator==(ServiceEndpoints const& lhs, ServiceEndpoints const& rhs);
}  // namespace launchdarkly::config
