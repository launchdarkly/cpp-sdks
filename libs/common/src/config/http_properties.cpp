#include <utility>

#include <launchdarkly/config/shared/built/http_properties.hpp>

namespace launchdarkly::config::shared::built {

TlsOptions::TlsOptions(bool verify_peer) : verify_peer_(verify_peer) {}

TlsOptions::TlsOptions() : TlsOptions(true) {}

bool TlsOptions::VerifyPeer() const {
    return verify_peer_;
}

HttpProperties::HttpProperties(std::chrono::milliseconds connect_timeout,
                               std::chrono::milliseconds read_timeout,
                               std::chrono::milliseconds write_timeout,
                               std::chrono::milliseconds response_timeout,
                               std::map<std::string, std::string> base_headers,
                               TlsOptions tls)
    : connect_timeout_(connect_timeout),
      read_timeout_(read_timeout),
      write_timeout_(write_timeout),
      response_timeout_(response_timeout),
      base_headers_(std::move(base_headers)),
      tls_(std::move(tls)) {}

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

bool operator==(HttpProperties const& lhs, HttpProperties const& rhs) {
    return lhs.ReadTimeout() == rhs.ReadTimeout() &&
           lhs.WriteTimeout() == rhs.WriteTimeout() &&
           lhs.ConnectTimeout() == rhs.ConnectTimeout() &&
           lhs.BaseHeaders() == rhs.BaseHeaders() && lhs.Tls() == rhs.Tls();
}

bool operator==(TlsOptions const& lhs, TlsOptions const& rhs) {
    return lhs.VerifyPeer() == rhs.VerifyPeer();
}

}  // namespace launchdarkly::config::shared::built
