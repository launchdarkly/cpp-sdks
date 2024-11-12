#include <launchdarkly/config/shared/builders/http_properties_builder.hpp>
#include <launchdarkly/config/shared/defaults.hpp>

#include <cstring>

namespace launchdarkly::config::shared::builders {

template <typename SDK>
TlsBuilder<SDK>::TlsBuilder() : TlsBuilder(shared::Defaults<SDK>::TLS()) {}

template <typename SDK>
TlsBuilder<SDK>::TlsBuilder(built::TlsOptions const& tls) {
    verify_mode_ = tls.PeerVerifyMode();
    custom_ca_file_ = tls.CustomCAFile();
}

template <typename SDK>
TlsBuilder<SDK>& TlsBuilder<SDK>::SkipVerifyPeer(bool skip_verify_peer) {
    verify_mode_ = skip_verify_peer
                       ? built::TlsOptions::VerifyMode::kVerifyNone
                       : built::TlsOptions::VerifyMode::kVerifyPeer;
    return *this;
}

template <typename SDK>
TlsBuilder<SDK>& TlsBuilder<SDK>::CustomCAFile(std::string custom_ca_file) {
    if (custom_ca_file.empty()) {
        custom_ca_file_ = std::nullopt;
    } else {
        custom_ca_file_ = std::move(custom_ca_file);
    }
    return *this;
}

template <typename SDK>
built::TlsOptions TlsBuilder<SDK>::Build() const {
    return {verify_mode_, custom_ca_file_};
}

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
    tls_ = properties.Tls();
    http_proxy_ = properties.HttpProxy();
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
HttpPropertiesBuilder<SDK>& HttpPropertiesBuilder<SDK>::WriteTimeout(
    std::chrono::milliseconds write_timeout) {
    write_timeout_ = write_timeout;
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
HttpPropertiesBuilder<SDK>& HttpPropertiesBuilder<SDK>::Headers(
    std::map<std::string, std::string> base_headers) {
    base_headers_ = std::move(base_headers);
    return *this;
}

template <typename SDK>
HttpPropertiesBuilder<SDK>& HttpPropertiesBuilder<SDK>::Header(
    std::string key,
    std::optional<std::string> value) {
    if (value) {
        base_headers_.insert_or_assign(key, *value);
    } else {
        base_headers_.erase(key);
    }
    return *this;
}

template <typename SDK>
HttpPropertiesBuilder<SDK>& HttpPropertiesBuilder<SDK>::Tls(
    TlsBuilder<SDK> builder) {
    tls_ = std::move(builder);
    return *this;
}

template <typename SDK>
template <typename T, std::enable_if_t<std::is_same_v<T, ClientSDK>, int>>
HttpPropertiesBuilder<SDK>& HttpPropertiesBuilder<SDK>::HttpProxy(
    std::string http_proxy) {
    http_proxy_ = std::move(http_proxy);
    return *this;
}

static std::optional<std::string> HttpProxyFromEnv() {
    if (char const* http_proxy = std::getenv("http_proxy")) {
        if (strlen(http_proxy) > 0) {
            return http_proxy;
        }
    }
    return std::nullopt;
}

template <typename SDK>
built::HttpProperties HttpPropertiesBuilder<SDK>::Build() const {
    std::map<std::string, std::string> headers = base_headers_;

    if (!wrapper_name_.empty()) {
        headers["X-LaunchDarkly-Wrapper"] =
            wrapper_name_ + "/" + wrapper_version_;
    }

    std::optional<std::string> http_proxy;

    if constexpr (std::is_same_v<SDK, ClientSDK>) {
        http_proxy = http_proxy_.has_value() ? http_proxy_ : HttpProxyFromEnv();
    }

    return {connect_timeout_,  read_timeout_,      write_timeout_,
            response_timeout_, std::move(headers), tls_.Build(),
            http_proxy};
}

template class TlsBuilder<config::shared::ClientSDK>;
template class TlsBuilder<config::shared::ServerSDK>;

template class HttpPropertiesBuilder<config::shared::ClientSDK>;
template class HttpPropertiesBuilder<config::shared::ServerSDK>;
}  // namespace launchdarkly::config::shared::builders
