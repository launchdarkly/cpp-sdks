#include "config/detail/service_hosts.hpp"

namespace launchdarkly::config {

ServiceHosts::ServiceHosts(std::string polling,
                           std::string streaming,
                           std::string events)
    : polling_host_(std::move(polling)),
      streaming_host_(std::move(streaming)),
      events_host_(std::move(events)) {}

std::string const& ServiceHosts::polling_host() const {
    return polling_host_;
}

std::string const& ServiceHosts::streaming_host() const {
    return streaming_host_;
}

std::string const& ServiceHosts::events_host() const {
    return events_host_;
}

bool operator==(ServiceHosts const& lhs, ServiceHosts const& rhs) {
    return lhs.polling_host() == rhs.polling_host() &&
           lhs.streaming_host() == rhs.streaming_host() &&
           lhs.events_host() == rhs.events_host();
}

}  // namespace launchdarkly::config
