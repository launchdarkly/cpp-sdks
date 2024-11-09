#pragma once

#include <chrono>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <launchdarkly/config/shared/built/http_properties.hpp>

namespace launchdarkly::config::shared::builders {

/**
 * Class used for building TLS options used within HttpProperties.
 * @tparam SDK The SDK type to build options for. This affects the default
 * values of the built options.
 */
template <typename SDK>
class TlsBuilder {
   public:
    /**
     * Construct a new TlsBuilder. The builder will use the default
     * properties based on the SDK type. Setting a property will override
     * the default value.
     */
    TlsBuilder();

    /**
     * Create a TLS builder from an initial set of options.
     * This can be useful when extending a set of options for a request.
     *
     * @param tls The TLS options to start with.
     */
    TlsBuilder(built::TlsOptions const& tls);

    /**
     * Whether to skip verifying the remote peer's certificates.
     * @param skip_verify_peer True to skip verification, false to verify.
     * @return A reference to this builder.
     */
    TlsBuilder& SkipVerifyPeer(bool skip_verify_peer);

    /**
     * Path to a file containing one or more CAs to verify
     * the peer with. The certificate(s) must be PEM-encoded.
     *
     * By default, the SDK uses the system's root CA bundle.
     *
     * If the empty string is passed, this function will clear any existing
     * CA bundle path previously set, and the system's root CA bundle will be
     * used.
     *
     * @param custom_ca_file File path.
     * @return A reference to this builder.
     */
    TlsBuilder& CustomCAFile(std::string custom_ca_file);

    /**
     * Builds the TLS options.
     * @return The built options.
     */
    [[nodiscard]] built::TlsOptions Build() const;

   private:
    enum built::TlsOptions::VerifyMode verify_mode_;
    std::optional<std::string> custom_ca_file_;
};
/**
 * Class used for building a set of HttpProperties.
 * @tparam SDK The SDK type to build properties for. This affects the default
 * values of the built properties.
 */
template <typename SDK>
class HttpPropertiesBuilder {
   public:
    /**
     * Construct a new HttpPropertiesBuilder. The builder will use the default
     * properties based on the SDK type. Setting a property will override
     * the default value.
     */
    HttpPropertiesBuilder();

    /**
     * Create a properties builder from an initial set of properties.
     * This can be useful when extending a set of properties for a request.
     * For instance to add extra headers.
     *
     * ```
     * HttpPropertiesBuilder(my_properties)
     *   .Header("authorization", "my-key")
     *   .Build();
     * ```
     *
     * @param properties The properties to start with.
     */
    HttpPropertiesBuilder(built::HttpProperties const& properties);

    /**
     * The network connection timeout.
     *
     * @param connect_timeout The connect timeout.
     * @return A reference to this builder.
     */
    HttpPropertiesBuilder& ConnectTimeout(
        std::chrono::milliseconds connect_timeout);

    /**
     * Set a read timeout. This is the time after the first byte
     * has been received that a read has to complete.
     *
     * @param read_timeout The read timeout.
     * @return A reference to this builder.
     */
    HttpPropertiesBuilder& ReadTimeout(std::chrono::milliseconds read_timeout);

    /**
     * Set a write timeout. This is how long it takes to perform a Write
     * operation.
     *
     * @param write_timeout The write timeout.
     * @return A reference to this builder.
     */
    HttpPropertiesBuilder& WriteTimeout(
        std::chrono::milliseconds write_timeout);

    /**
     * The time for the first byte to be received during a read. If a byte
     * is not received within this time, then the request will be cancelled.
     *
     * @param response_timeout The response timeout.
     * @return A reference to this builder.
     */
    HttpPropertiesBuilder& ResponseTimeout(
        std::chrono::milliseconds response_timeout);

    /**
     * This should be used for wrapper SDKs to set the wrapper name.
     *
     * Wrapper information will be included in request headers.
     * @param wrapper_name The name of the wrapper.
     * @return A reference to this builder.
     */
    HttpPropertiesBuilder& WrapperName(std::string wrapper_name);

    /**
     * This should be used for wrapper SDKs to set the wrapper version.
     *
     * Wrapper information will be included in request headers.
     * @param wrapper_version The version of the wrapper.
     * @return A reference to this builder.
     */
    HttpPropertiesBuilder& WrapperVersion(std::string wrapper_version);

    /**
     * Set all custom headers. This will replace any other customer headers
     * that were set with the Header method, or any previously set
     * headers using the Headers method.
     * @param base_headers The custom headers.
     * @return A reference to this builder.
     */
    HttpPropertiesBuilder& Headers(
        std::map<std::string, std::string> base_headers);

    /**
     * Set an optional header value. If the value is std::nullopt, any existing
     * header by that name is removed.
     * @param name The name of the header.
     * @param value The optional header value.
     * @return A reference to this builder.
     */
    HttpPropertiesBuilder& Header(std::string key,
                                  std::optional<std::string> value);

    /**
     * Sets the builder for TLS properties.
     * @param builder The TLS property builder.
     * @return A reference to this builder.
     */
    HttpPropertiesBuilder& Tls(TlsBuilder<SDK> builder);

    /**
     * Sets an HTTP proxy URL.
     * @param http_proxy The proxy, for example 'http://proxy.example.com:8080'.
     * @return A reference to this builder.
     */
    HttpPropertiesBuilder& HttpProxy(std::string http_proxy);

    /**
     * Build a set of HttpProperties.
     * @return The built properties.
     */
    [[nodiscard]] built::HttpProperties Build() const;

   private:
    std::chrono::milliseconds connect_timeout_;
    std::chrono::milliseconds read_timeout_;
    std::chrono::milliseconds write_timeout_;
    std::chrono::milliseconds response_timeout_;
    std::string wrapper_name_;
    std::string wrapper_version_;
    std::map<std::string, std::string> base_headers_;
    TlsBuilder<SDK> tls_;
    std::optional<std::string> http_proxy_;
};

}  // namespace launchdarkly::config::shared::builders
