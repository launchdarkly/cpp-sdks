#include "config/service_endpoints_builder.hpp"

#include <utility>

namespace launchdarkly::config {

template <typename SDK>
ServiceEndpointsBuilder<SDK>::ServiceEndpointsBuilder()
    : polling_base_url_(),
      streaming_base_url_(),
      events_base_url_(),
      defaults_() {}

template <typename SDK>
ServiceEndpointsBuilder<SDK>& ServiceEndpointsBuilder<SDK>::polling_base_url(
    std::string url) {
    polling_base_url_ = std::move(url);
    return *this;
}
template <typename SDK>
ServiceEndpointsBuilder<SDK>& ServiceEndpointsBuilder<SDK>::streaming_base_url(
    std::string url) {
    streaming_base_url_ = std::move(url);
    return *this;
}
template <typename SDK>
ServiceEndpointsBuilder<SDK>& ServiceEndpointsBuilder<SDK>::events_base_url(
    std::string url) {
    events_base_url_ = std::move(url);
    return *this;
}

template <typename SDK>
ServiceEndpointsBuilder<SDK>& ServiceEndpointsBuilder<SDK>::relay_proxy(
    std::string const& url) {
    return polling_base_url(url).streaming_base_url(url).events_base_url(url);
}

template <typename SDK>
std::unique_ptr<ServiceEndpoints> ServiceEndpointsBuilder<SDK>::build() const {
    if (!polling_base_url_ && !streaming_base_url_ && !events_base_url_) {
        return defaults_.endpoints();
    }
    if (polling_base_url_ && streaming_base_url_ && events_base_url_) {
    }
    return nullptr;
}

}  // namespace launchdarkly::config
