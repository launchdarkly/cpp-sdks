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

/**
 * Proxy configuration for HTTP requests.
 *
 * When using CURL networking (LD_CURL_NETWORKING=ON), this controls proxy behavior:
 * - std::nullopt (default): CURL uses environment variables (ALL_PROXY, HTTP_PROXY, HTTPS_PROXY)
 * - Non-empty string: Explicitly configured proxy URL takes precedence over environment variables
 * - Empty string: Explicitly disables proxy, preventing environment variable usage
 *
 * The empty string is forwarded to the networking implementation (CURL) which interprets
 * it as "do not use any proxy, even if environment variables are set."
 *
 * When CURL networking is disabled, attempting to configure a proxy will throw an error.
 */
class ProxyOptions final {
   public:
    /**
     * Construct proxy options with a proxy URL.
     *
     * @param url Proxy URL or configuration:
     *            - std::nullopt: Use environment variables (default)
     *            - "socks5://user:pass@proxy.example.com:1080": SOCKS5 proxy with auth
     *            - "socks5h://proxy:1080": SOCKS5 proxy with DNS resolution through proxy
     *            - "http://proxy.example.com:8080": HTTP proxy
     *            - "": Empty string explicitly disables proxy (overrides environment variables)
     *
     * @throws std::runtime_error if proxy URL is non-empty and CURL networking is not enabled
     */
    explicit ProxyOptions(std::optional<std::string> url);

    /**
     * Default constructor. Uses environment variables for proxy configuration.
     */
    ProxyOptions();

    /**
     * Get the configured proxy URL.
     * @return Proxy URL if configured, or std::nullopt to use environment variables.
     */
    [[nodiscard]] std::optional<std::string> const& Url() const;

   private:
    std::optional<std::string> url_;
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
bool operator==(TlsOptions const& lhs, TlsOptions const& rhs);
bool operator==(ProxyOptions const& lhs, ProxyOptions const& rhs);

}  // namespace launchdarkly::config::shared::built
