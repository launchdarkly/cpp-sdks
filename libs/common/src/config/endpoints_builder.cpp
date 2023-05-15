#include <launchdarkly/config/shared/builders/endpoints_builder.hpp>
#include "launchdarkly/config/shared/defaults.hpp"

#include <utility>

namespace launchdarkly::config::shared::builders {

template <typename SDK>
EndpointsBuilder<SDK>& EndpointsBuilder<SDK>::PollingBaseUrl(std::string url) {
    polling_base_url_ = std::move(url);
    return *this;
}
template <typename SDK>
EndpointsBuilder<SDK>& EndpointsBuilder<SDK>::StreamingBaseUrl(
    std::string url) {
    streaming_base_url_ = std::move(url);
    return *this;
}
template <typename SDK>
EndpointsBuilder<SDK>& EndpointsBuilder<SDK>::EventsBaseUrl(std::string url) {
    events_base_url_ = std::move(url);
    return *this;
}

template <typename SDK>
EndpointsBuilder<SDK>& EndpointsBuilder<SDK>::RelayProxy(
    std::string const& url) {
    return PollingBaseUrl(url).StreamingBaseUrl(url).EventsBaseUrl(url);
}

std::string trim_right_matches(std::string const& input, char match) {
    size_t end = input.size();
    for (; end > 0; end--) {
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
tl::expected<built::ServiceEndpoints, Error> EndpointsBuilder<SDK>::Build() {
    // Empty URLs are not allowed.
    if (empty_string(polling_base_url_) || empty_string(streaming_base_url_) ||
        empty_string(events_base_url_)) {
        return tl::unexpected(Error::kConfig_Endpoints_EmptyURL);
    }

    // If no URLs were set, return the default endpoints for this SDK.
    if (!polling_base_url_ && !streaming_base_url_ && !events_base_url_) {
        return shared::Defaults<SDK>::ServiceEndpoints();
    }

    // If all URLs were set, trim any trailing slashes and construct custom
    // ServiceEndpoints.
    if (polling_base_url_ && streaming_base_url_ && events_base_url_) {
        auto trim_trailing_slashes = [](std::string const& url) -> std::string {
            return trim_right_matches(url, '/');
        };
        return built::ServiceEndpoints(
            trim_trailing_slashes(*polling_base_url_),
            trim_trailing_slashes(*streaming_base_url_),
            trim_trailing_slashes(*events_base_url_));
    }

    // Otherwise if a subset of URLs were set, this is an error.
    return tl::unexpected(Error::kConfig_Endpoints_AllURLsMustBeSet);
}

template <typename SDK>
bool operator==(EndpointsBuilder<SDK> const& lhs,
                EndpointsBuilder<SDK> const& rhs) {
    return lhs.events_base_url_ == rhs.events_base_url_ &&
           lhs.streaming_base_url_ == rhs.streaming_base_url_ &&
           lhs.polling_base_url_ == rhs.polling_base_url_;
}

template class EndpointsBuilder<config::shared::ClientSDK>;
template class EndpointsBuilder<config::shared::ServerSDK>;

template bool operator==(EndpointsBuilder<config::shared::ClientSDK> const&,
                         EndpointsBuilder<config::shared::ClientSDK> const&);

template bool operator==(EndpointsBuilder<config::shared::ServerSDK> const&,
                         EndpointsBuilder<config::shared::ServerSDK> const&);

}  // namespace launchdarkly::config::shared::builders
