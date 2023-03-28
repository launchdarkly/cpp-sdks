#pragma once

#include <string>

namespace launchdarkly::config {

class ServiceEndpoints {
   private:
    std::string polling_base_url_;
    std::string streaming_base_url_;
    std::string events_base_url_;

   public:
    ServiceEndpoints(std::string polling,
                     std::string streaming,
                     std::string events);
    [[nodiscard]] std::string const& polling_base_url() const;
    [[nodiscard]] std::string const& streaming_base_url() const;
    [[nodiscard]] std::string const& events_base_url() const;
};

}  // namespace launchdarkly::config
