#pragma once

#include <chrono>
#include <map>
#include <string>
#include <vector>

#include <launchdarkly/config/shared/built/http_properties.hpp>

namespace launchdarkly::config::detail::builders {

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
     * @param read_timeout The Write timeout.
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
     * headers using the CustomHeaders method.
     * @param base_headers The custom headers.
     * @return A reference to this builder.
     */
    HttpPropertiesBuilder& CustomHeaders(
        std::map<std::string, std::string> base_headers);

    /**
     * Set a custom header value.
     *
     * Calling CustomHeaders will replace any previously set values.
     * @param key The key for the header.
     * @param value The header value.
     * @return A reference to this builder.
     */
    HttpPropertiesBuilder& Header(std::string key, std::string value);

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
    std::string user_agent_;
};

}  // namespace launchdarkly::config::detail::builders
