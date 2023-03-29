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

std::string trim_right_matches(std::string const& input, char match) {
    size_t end;
    for (end = input.size(); end > 0; end--) {
        if (input.at(end - 1) != match) {
            break;
        }
    }
    return input.substr(0, end);
}

bool empty_string(std::optional<std::string> const& opt_string) {
    return opt_string.has_value() && opt_string->empty();
}

template <typename SDK>
std::unique_ptr<ServiceEndpoints> EndpointsBuilder<SDK>::build() {
    // Empty URLs are not allowed.
    if (empty_string(polling_base_url_) || empty_string(streaming_base_url_) ||
        empty_string(events_base_url_)) {
        return nullptr;
    }

    // If no URLs were set, return the default endpoints for this SDK.
    if (!polling_base_url_ && !streaming_base_url_ && !events_base_url_) {
        return std::make_unique<ServiceEndpoints>(
            detail::Defaults<SDK>::endpoints());
    }

    // If all URLs were set, trim any trailing slashes and construct custom
    // ServiceEndpoints.
    if (polling_base_url_ && streaming_base_url_ && events_base_url_) {
        auto trim_trailing_slashes = [](std::string const& s) -> std::string {
            return trim_right_matches(s, '/');
        };
        return std::make_unique<ServiceEndpoints>(
            trim_trailing_slashes(*polling_base_url_),
            trim_trailing_slashes(*streaming_base_url_),
            trim_trailing_slashes(*events_base_url_));
    }

    // Otherwise if a subset of URLs were set, this is an error.
    return nullptr;
}

template class EndpointsBuilder<detail::ClientSDK>;
template class EndpointsBuilder<detail::ServerSDK>;

}  // namespace launchdarkly::config::detail
