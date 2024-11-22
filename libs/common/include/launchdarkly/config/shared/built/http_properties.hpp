#pragma once

#include <chrono>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace launchdarkly::config::shared::built {

class TlsOptions final {
   public:
    enum class VerifyMode { kVerifyPeer, kVerifyNone };
    explicit TlsOptions(VerifyMode verify_mode);
    TlsOptions(VerifyMode verify_mode,
               std::optional<std::string> ca_bundle_path);
    TlsOptions();
    [[nodiscard]] VerifyMode PeerVerifyMode() const;
    [[nodiscard]] std::optional<std::string> const& CustomCAFile() const;

   private:
    VerifyMode verify_mode_;
    std::optional<std::string> ca_bundle_path_;
};

class ProxyOptions final {
   public:
    ProxyOptions(std::optional<std::string> http_proxy,
                 std::optional<std::string> https_proxy);

    ProxyOptions() = default;
    [[nodiscard]] std::optional<std::string> Http() const;
    [[nodiscard]] std::optional<std::string> Https() const;

   private:
    std::optional<std::string> http_proxy_;
    std::optional<std::string> https_proxy_;
};

class HttpProperties final {
   public:
    HttpProperties(std::chrono::milliseconds connect_timeout,
                   std::chrono::milliseconds read_timeout,
                   std::chrono::milliseconds write_timeout,
                   std::chrono::milliseconds response_timeout,
                   std::map<std::string, std::string> base_headers,
                   TlsOptions tls,
                   ProxyOptions proxy);

    [[nodiscard]] std::chrono::milliseconds ConnectTimeout() const;
    [[nodiscard]] std::chrono::milliseconds ReadTimeout() const;
    [[nodiscard]] std::chrono::milliseconds WriteTimeout() const;

    [[nodiscard]] std::chrono::milliseconds ResponseTimeout() const;
    [[nodiscard]] std::map<std::string, std::string> const& BaseHeaders() const;

    [[nodiscard]] TlsOptions const& Tls() const;

    [[nodiscard]] ProxyOptions const& Proxy() const;

   private:
    std::chrono::milliseconds connect_timeout_;
    std::chrono::milliseconds read_timeout_;
    std::chrono::milliseconds write_timeout_;
    std::chrono::milliseconds response_timeout_;
    std::map<std::string, std::string> base_headers_;
    TlsOptions tls_;
    ProxyOptions proxy_;
};

bool operator==(HttpProperties const& lhs, HttpProperties const& rhs);
bool operator==(ProxyOptions const& lhs, ProxyOptions const& rhs);
bool operator==(TlsOptions const& lhs, TlsOptions const& rhs);

}  // namespace launchdarkly::config::shared::built
