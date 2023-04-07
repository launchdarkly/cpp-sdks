#pragma once

#include <string>

namespace launchdarkly::config {

/**
 * ServiceEndpoints contains configured endpoints for the LaunchDarkly
 * service or a Relay Proxy instance.
 */
class ServiceHosts {
   public:
    /**
     * Constructs a ServiceHosts from individual polling, streaming, and
     * events hosts.
     *
     * These are bare hostname + ports; no schema or paths (e.g.
     * stream.launchdarkly.com).
     *
     * Meant for internal usage only; see ClientHostsBuilder or
     * ServerHostsBuilder for their respective SDK usage.
     *
     * @param polling Polling host.
     * @param streaming Streaming host.
     * @param events Events host.
     */
    ServiceHosts(std::string polling,
                 std::string streaming,
                 std::string events);

    [[nodiscard]] std::string const& polling_host() const;
    [[nodiscard]] std::string const& streaming_host() const;
    [[nodiscard]] std::string const& events_host() const;

   private:
    std::string polling_host_;
    std::string streaming_host_;
    std::string events_host_;
};

bool operator==(ServiceHosts const& lhs, ServiceHosts const& rhs);
}  // namespace launchdarkly::config
