#include <utility>

#include <launchdarkly/config/shared/built/http_properties.hpp>

namespace launchdarkly::config::shared::built {

TlsOptions::TlsOptions(VerifyMode const verify_mode,
                       std::optional<std::string> ca_bundle_path)
    : verify_mode_(verify_mode), ca_bundle_path_(std::move(ca_bundle_path)) {}

TlsOptions::TlsOptions(VerifyMode const verify_mode)
    : TlsOptions(verify_mode, std::nullopt) {}

TlsOptions::TlsOptions() : TlsOptions(VerifyMode::kVerifyPeer, std::nullopt) {}

TlsOptions::VerifyMode TlsOptions::PeerVerifyMode() const {
    return verify_mode_;
}

std::optional<std::string> const& TlsOptions::CustomCAFile() const {
    return ca_bundle_path_;
}

HttpProperties::HttpProperties(std::chrono::milliseconds connect_timeout,
                               std::chrono::milliseconds read_timeout,
                               std::chrono::milliseconds write_timeout,
                               std::chrono::milliseconds response_timeout,
                               std::map<std::string, std::string> base_headers,
                               TlsOptions tls,
                               std::optional<std::string> http_proxy)
    : connect_timeout_(connect_timeout),
      read_timeout_(read_timeout),
      write_timeout_(write_timeout),
      response_timeout_(response_timeout),
      base_headers_(std::move(base_headers)),
      tls_(std::move(tls)),
      http_proxy_(std::move(http_proxy)) {}

std::chrono::milliseconds HttpProperties::ConnectTimeout() const {
    return connect_timeout_;
}

std::chrono::milliseconds HttpProperties::ReadTimeout() const {
    return read_timeout_;
}

std::chrono::milliseconds HttpProperties::WriteTimeout() const {
    return write_timeout_;
}

std::chrono::milliseconds HttpProperties::ResponseTimeout() const {
    return response_timeout_;
}

std::map<std::string, std::string> const& HttpProperties::BaseHeaders() const {
    return base_headers_;
}

TlsOptions const& HttpProperties::Tls() const {
    return tls_;
}

std::optional<std::string> HttpProperties::HttpProxy() const {
    return http_proxy_;
}

bool operator==(HttpProperties const& lhs, HttpProperties const& rhs) {
    return lhs.ReadTimeout() == rhs.ReadTimeout() &&
           lhs.WriteTimeout() == rhs.WriteTimeout() &&
           lhs.ConnectTimeout() == rhs.ConnectTimeout() &&
           lhs.BaseHeaders() == rhs.BaseHeaders() && lhs.Tls() == rhs.Tls() &&
           lhs.HttpProxy() == rhs.HttpProxy();
}

bool operator==(TlsOptions const& lhs, TlsOptions const& rhs) {
    return lhs.PeerVerifyMode() == rhs.PeerVerifyMode() &&
           lhs.CustomCAFile() == rhs.CustomCAFile();
}

}  // namespace launchdarkly::config::shared::built
