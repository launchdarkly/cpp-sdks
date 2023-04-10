#include "config/detail/hosts_builder.hpp"
#include "config/detail/defaults.hpp"

#include <utility>

namespace launchdarkly::config::detail {

template <typename SDK>
HostsBuilder<SDK>& HostsBuilder<SDK>::polling_host(std::string url) {
    polling_host_ = std::move(url);
    return *this;
}

template <typename SDK>
HostsBuilder<SDK>& HostsBuilder<SDK>::streaming_host(std::string url) {
    streaming_host_ = std::move(url);
    return *this;
}

template <typename SDK>
HostsBuilder<SDK>& HostsBuilder<SDK>::events_host(std::string url) {
    events_host_ = std::move(url);
    return *this;
}

template <typename SDK>
HostsBuilder<SDK>& HostsBuilder<SDK>::relay_proxy(std::string const& url) {
    return polling_host(url).streaming_host(url).events_host(url);
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
tl::expected<ServiceHosts, Error> HostsBuilder<SDK>::build() {
    // Empty hostnames are not allowed.
    if (empty_string(polling_host_) || empty_string(streaming_host_) ||
        empty_string(events_host_)) {
        return tl::unexpected(Error::kConfig_Endpoints_EmptyURL);
    }

    // If no hosts were set, return the default hosts for this SDK.
    if (!polling_host_ && !streaming_host_ && !events_host_) {
        return detail::Defaults<SDK>::endpoints();
    }

    // If all hosts were set, trim any trailing slashes - this is a common
    // mistake, and can be automatically corrected.
    if (polling_host_ && streaming_host_ && events_host_) {
        auto trim_trailing_slashes = [](std::string const& url) -> std::string {
            return trim_right_matches(url, '/');
        };
        return ServiceHosts(trim_trailing_slashes(*polling_host_),
                            trim_trailing_slashes(*streaming_host_),
                            trim_trailing_slashes(*events_host_));
    }

    // Otherwise if a subset of URLs were set, this is an error.
    return tl::unexpected(Error::kConfig_Endpoints_AllURLsMustBeSet);
}

template <typename SDK>
bool operator==(HostsBuilder<SDK> const& lhs, HostsBuilder<SDK> const& rhs) {
    return lhs.events_host_ == rhs.events_host_ &&
           lhs.streaming_host_ == rhs.streaming_host_ &&
           lhs.polling_host_ == rhs.polling_host_;
}

template class HostsBuilder<detail::ClientSDK>;
template class HostsBuilder<detail::ServerSDK>;

template bool operator==(HostsBuilder<detail::ClientSDK> const&,
                         HostsBuilder<detail::ClientSDK> const&);

template bool operator==(HostsBuilder<detail::ServerSDK> const&,
                         HostsBuilder<detail::ServerSDK> const&);

}  // namespace launchdarkly::config::detail
