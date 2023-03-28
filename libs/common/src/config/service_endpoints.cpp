#include "config/service_endpoints.hpp"

namespace launchdarkly::config {

ServiceEndpoints::ServiceEndpoints(std::string polling,
                                   std::string streaming,
                                   std::string events)
    : polling_base_url_(std::move(polling)),
      streaming_base_url_(std::move(streaming)),
      events_base_url_(std::move(events)) {}

std::string const& ServiceEndpoints::polling_base_url() const {
    return polling_base_url_;
}

std::string const& ServiceEndpoints::streaming_base_url() const {
    return streaming_base_url_;
}

std::string const& ServiceEndpoints::events_base_url() const {
    return events_base_url_;
}
}  // namespace launchdarkly::config
