#include "config/detail/service_endpoints.hpp"

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

bool operator==(ServiceEndpoints const& lhs, ServiceEndpoints const& rhs) {
    return lhs.polling_base_url() == rhs.polling_base_url() &&
           lhs.streaming_base_url() == rhs.streaming_base_url() &&
           lhs.events_base_url() == rhs.events_base_url();
}

}  // namespace launchdarkly::config
