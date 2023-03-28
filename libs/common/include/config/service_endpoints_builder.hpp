#pragma once

#include "defaults.hpp"

#include <memory>
#include <optional>
#include <string>

namespace launchdarkly::config {

template <typename SDK>
class ServiceEndpointsBuilder {
   private:
    std::optional<std::string> polling_base_url_;
    std::optional<std::string> streaming_base_url_;
    std::optional<std::string> events_base_url_;

    Defaults<SDK> defaults_;

   public:
    ServiceEndpointsBuilder();
    ServiceEndpointsBuilder& polling_base_url(std::string url);
    ServiceEndpointsBuilder& streaming_base_url(std::string url);
    ServiceEndpointsBuilder& events_base_url(std::string url);
    ServiceEndpointsBuilder& relay_proxy(std::string const& url);
    std::unique_ptr<ServiceEndpoints> build() const;
};

}  // namespace launchdarkly::config
