#pragma once

#include "config/service_endpoints.hpp"

#include <optional>
#include <string>

namespace launchdarkly::config::detail {
template <typename SDK>
class EndpointsBuilder {
   private:
    std::optional<std::string> polling_base_url_;
    std::optional<std::string> streaming_base_url_;
    std::optional<std::string> events_base_url_;

   public:
    EndpointsBuilder();
    EndpointsBuilder& polling_base_url(std::string url);
    EndpointsBuilder& streaming_base_url(std::string url);
    EndpointsBuilder& events_base_url(std::string url);
    EndpointsBuilder& relay_proxy(std::string const& url);
    [[nodiscard]] std::unique_ptr<ServiceEndpoints> build();
};

}  // namespace launchdarkly::config::detail
