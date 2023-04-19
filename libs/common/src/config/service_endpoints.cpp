#include "config/detail/built/service_endpoints.hpp"

namespace launchdarkly::config::detail::built {

ServiceEndpoints::ServiceEndpoints(std::string polling,
                                   std::string streaming,
                                   std::string events)
    : polling_base_url_(std::move(polling)),
      streaming_base_url_(std::move(streaming)),
      events_base_url_(std::move(events)) {}

std::string const& ServiceEndpoints::PollingBaseUrl() const {
    return polling_base_url_;
}

std::string const& ServiceEndpoints::StreamingBaseUrl() const {
    return streaming_base_url_;
}

std::string const& ServiceEndpoints::EventsBaseUrl() const {
    return events_base_url_;
}

bool operator==(ServiceEndpoints const& lhs, ServiceEndpoints const& rhs) {
    return lhs.PollingBaseUrl() == rhs.PollingBaseUrl() &&
           lhs.StreamingBaseUrl() == rhs.StreamingBaseUrl() &&
           lhs.EventsBaseUrl() == rhs.EventsBaseUrl();
}

}  // namespace launchdarkly::config::detail::built
