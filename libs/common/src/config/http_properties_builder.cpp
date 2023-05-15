#include <utility>

#include <launchdarkly/config/shared/builders/http_properties_builder.hpp>
#include <launchdarkly/config/shared/defaults.hpp>
#include <launchdarkly/config/shared/sdks.hpp>

namespace launchdarkly::config::shared::builders {

template <typename SDK>
HttpPropertiesBuilder<SDK>::HttpPropertiesBuilder()
    : HttpPropertiesBuilder(shared::Defaults<SDK>::HttpProperties()) {}

template <typename SDK>
HttpPropertiesBuilder<SDK>::HttpPropertiesBuilder(
    built::HttpProperties const& properties) {
    connect_timeout_ = properties.ConnectTimeout();
    read_timeout_ = properties.ReadTimeout();
    write_timeout_ = properties.WriteTimeout();
    response_timeout_ = properties.ResponseTimeout();
    base_headers_ = properties.BaseHeaders();
    user_agent_ = properties.UserAgent();
}

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
HttpPropertiesBuilder<SDK>& HttpPropertiesBuilder<SDK>::ResponseTimeout(
    std::chrono::milliseconds response_timeout) {
    response_timeout_ = response_timeout;
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
HttpPropertiesBuilder<SDK>& HttpPropertiesBuilder<SDK>::Header(
    std::string key,
    std::string value) {
    base_headers_.insert_or_assign(key, value);
    return *this;
}

template <typename SDK>
built::HttpProperties HttpPropertiesBuilder<SDK>::Build() const {
    if (!wrapper_name_.empty()) {
        std::map<std::string, std::string> headers_with_wrapper(base_headers_);
        headers_with_wrapper["X-LaunchDarkly-Wrapper"] =
            wrapper_name_ + "/" + wrapper_version_;
        return {connect_timeout_,  read_timeout_, write_timeout_,
                response_timeout_, user_agent_,   headers_with_wrapper};
    }
    return {connect_timeout_,  read_timeout_, write_timeout_,
            response_timeout_, user_agent_,   base_headers_};
}

template class HttpPropertiesBuilder<config::shared::ClientSDK>;
template class HttpPropertiesBuilder<config::shared::ServerSDK>;
}  // namespace launchdarkly::config::shared::builders
