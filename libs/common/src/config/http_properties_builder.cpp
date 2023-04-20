#include <utility>

#include "config/detail/builders/http_properties_builder.hpp"
#include "config/detail/defaults.hpp"
#include "config/detail/sdks.hpp"

namespace launchdarkly::config::detail::builders {

template <typename SDK>
HttpPropertiesBuilder<SDK>& HttpPropertiesBuilder<SDK>::ConnectTimeout(
    std::chrono::milliseconds connect_timeout) {
    connect_timeout_ = connect_timeout;
    return *this;
}

template <typename SDK>
HttpPropertiesBuilder<SDK>& HttpPropertiesBuilder<SDK>::ReadTimeout(
    std::chrono::milliseconds read_timeout) {
    read_timeout_ = read_timeout;
    return *this;
}

template <typename SDK>
HttpPropertiesBuilder<SDK>& HttpPropertiesBuilder<SDK>::WrapperName(
    std::string wrapper_name) {
    wrapper_name_ = std::move(wrapper_name);
    return *this;
}

template <typename SDK>
HttpPropertiesBuilder<SDK>& HttpPropertiesBuilder<SDK>::WrapperVersion(
    std::string wrapper_version) {
    wrapper_version_ = std::move(wrapper_version);
    return *this;
}

template <typename SDK>
HttpPropertiesBuilder<SDK>& HttpPropertiesBuilder<SDK>::CustomHeaders(
    std::map<std::string, std::string> base_headers) {
    base_headers_ = std::move(base_headers);
    return *this;
}

template <typename SDK>
built::HttpProperties HttpPropertiesBuilder<SDK>::Build() const {
    if (!wrapper_name_.empty()) {
        std::map<std::string, std::string> headers_with_wrapper(base_headers_);
        headers_with_wrapper["X-LaunchDarkly-Wrapper"] =
            wrapper_name_ + "/" + wrapper_version_;
        return {connect_timeout_, read_timeout_,
                detail::Defaults<SDK>::HttpProperties().UserAgent(),
                headers_with_wrapper};
    }
    return {connect_timeout_, read_timeout_, "", base_headers_};
}

template class HttpPropertiesBuilder<detail::ClientSDK>;
template class HttpPropertiesBuilder<detail::ServerSDK>;
}  // namespace launchdarkly::config::detail::builders
