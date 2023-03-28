#include "config/detail/defaults.hpp"
#include "config/detail/endpoints_builder.hpp"

#include <utility>

namespace launchdarkly::config::detail {

template <typename SDK>
EndpointsBuilder<SDK>::EndpointsBuilder()
    : polling_base_url_(), streaming_base_url_(), events_base_url_() {}

template <typename SDK>
EndpointsBuilder<SDK>& EndpointsBuilder<SDK>::polling_base_url(
    std::string url) {
    polling_base_url_ = std::move(url);
    return *this;
}
template <typename SDK>
EndpointsBuilder<SDK>& EndpointsBuilder<SDK>::streaming_base_url(
    std::string url) {
    streaming_base_url_ = std::move(url);
    return *this;
}
template <typename SDK>
EndpointsBuilder<SDK>& EndpointsBuilder<SDK>::events_base_url(std::string url) {
    events_base_url_ = std::move(url);
    return *this;
}

template <typename SDK>
EndpointsBuilder<SDK>& EndpointsBuilder<SDK>::relay_proxy(
    std::string const& url) {
    return polling_base_url(url).streaming_base_url(url).events_base_url(url);
}

template <typename SDK>
std::unique_ptr<ServiceEndpoints> EndpointsBuilder<SDK>::build() {
    if (!polling_base_url_ && !streaming_base_url_ && !events_base_url_) {
        return std::make_unique<ServiceEndpoints>(
            detail::Defaults<SDK>::endpoints());
    }
    if (polling_base_url_ && streaming_base_url_ && events_base_url_) {
        return std::make_unique<ServiceEndpoints>(
            *polling_base_url_, *streaming_base_url_, *events_base_url_);
    }
    return nullptr;
}

template class EndpointsBuilder<detail::ClientSDK>;
template class EndpointsBuilder<detail::ServerSDK>;

}  // namespace launchdarkly::config::detail
