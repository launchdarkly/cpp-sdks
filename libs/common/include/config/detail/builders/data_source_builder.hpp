#pragma once

#include <chrono>
#include <optional>
#include <type_traits>

#include "config/detail/built/data_source_config.hpp"
#include "config/detail/defaults.hpp"
#include "config/detail/sdks.hpp"

#include <boost/variant.hpp>

namespace launchdarkly::config::detail::builders {

/**
 * Used to construct a DataSourceConfiguration for the specified SDK type.
 * @tparam SDK ClientSDK or ServerSDK.
 */
template <typename SDK>
class DataSourceBuilder;

/**
 * Builds a configuration for a streaming data source.
 */
class StreamingBuilder {
   public:
    StreamingBuilder();

    /**
     * Sets the initial reconnect delay for the streaming connection.
     *
     * The streaming service uses a backoff algorithm (with jitter) every time
     * the connection needs to be reestablished.The delay for the first
     * reconnection will start near this value, and then increase exponentially
     * for any subsequent connection failures.
     *
     * @param initial_reconnect_delay The initial delay for a reconnection
     * attempt.
     * @return Reference to this builder.
     */
    StreamingBuilder& InitialReconnectDelay(
        std::chrono::milliseconds initial_reconnect_delay);

    /**
     * Build the streaming config. Used internal to the SDK.
     * @return The built config.
     */
    [[nodiscard]] built::StreamingConfig Build() const;

   private:
    built::StreamingConfig config_;
};

/**
 * Contains methods for configuring the polling data source.
 */
class PollingBuilder {
   public:
    PollingBuilder();

    /**
     * Sets the interval at which the SDK will poll for feature flag updates.
     * @param poll_interval The polling interval.
     * @return Reference to this builder.
     */
    PollingBuilder& poll_interval(std::chrono::seconds poll_interval);

    /**
     * Build the polling config. Used internal to the SDK.
     * @return The built config.
     */
    [[nodiscard]] built::PollingConfig Build() const;

   private:
    built::PollingConfig config_;
};

/**
 * The method visitor is only needed inside this file
 */
namespace {
struct MethodVisitor {
    boost::variant<built::StreamingConfig, built::PollingConfig> operator()(
        StreamingBuilder streaming) {
        return streaming.Build();
    }
    boost::variant<built::StreamingConfig, built::PollingConfig> operator()(
        PollingBuilder polling) {
        return polling.Build();
    }
};
}  // namespace

template <>
class DataSourceBuilder<ClientSDK> {
   public:
    using Streaming = StreamingBuilder;
    using Polling = PollingBuilder;

    DataSourceBuilder();

    /**
     * Whether LaunchDarkly should provide additional information about how flag
     * values were calculated.
     *
     * The additional information will then be available through the client's
     * {TODO variation detail} method. Since this increases the size of network
     * requests, such information is not sent unless you set this option to
     * true.
     * @param value True to enable reasons.
     * @return Reference to this builder.
     */
    DataSourceBuilder& with_reasons(bool value);

    /**
     * Whether or not to use the REPORT verb to fetch flag settings.
     *
     * If this is true, flag settings will be fetched with a REPORT request
     * including a JSON entity body with the context object.
     *
     * Otherwise (by default) a GET request will be issued with the context
     * passed as a base64 URL-encoded path parameter.
     *
     * Do not use unless advised by LaunchDarkly.
     * @param value True to enable using the REPORT verb.
     * @return Reference to this builder.
     */
    DataSourceBuilder& use_report(bool value);

    /**
     * Set the streaming configuration for the builder.
     *
     * A data source may either be streaming or polling. Setting a streaming
     * builder indicates the data source will use streaming. Setting a polling
     * builder will indicate the use of polling.
     *
     * @param stream_builder The streaming builder.
     * @return Reference to this builder.
     */
    DataSourceBuilder& method(Streaming stream_builder);

    /**
     * Set the polling configuration for the builder.
     *
     * A data source may either be streaming or polling. Setting a stream
     * builder indicates the data source will use streaming. Setting a polling
     * builder will indicate the use of polling.
     *
     * @param polling_builder The polling builder.
     * @return Reference to this builder.
     */
    DataSourceBuilder& method(Polling polling_builder);

    /**
     * Build a data source config. This is used internal to the SDK.
     *
     * @return The built config.
     */
    [[nodiscard]] built::DataSourceConfig<ClientSDK> Build() const;

   private:
    boost::variant<Streaming, Polling> method_;
    bool with_reasons_;
    bool use_report_;
};

template <>
class DataSourceBuilder<ServerSDK> {
   public:
    using Streaming = StreamingBuilder;
    using Polling = PollingBuilder;

    DataSourceBuilder();

    /**
     * Set the streaming configuration for the builder.
     *
     * A data source may either be streaming or polling. Setting a streaming
     * builder indicates the data source will use streaming. Setting a polling
     * builder will indicate the use of polling.
     *
     * @param stream_builder The streaming builder.
     * @return Reference to this builder.
     */
    DataSourceBuilder& method(Streaming builder);

    /**
     * Set the polling configuration for the builder.
     *
     * A data source may either be streaming or polling. Setting a stream
     * builder indicates the data source will use streaming. Setting a polling
     * builder will indicate the use of polling.
     *
     * @param polling_builder The polling builder.
     * @return Reference to this builder.
     */
    DataSourceBuilder& method(Polling builder);

    /**
     * Build a data source config. This is used internal to the SDK.
     *
     * @return The built config.
     */
    [[nodiscard]] built::DataSourceConfig<ServerSDK> Build() const;

   private:
    boost::variant<Streaming, Polling> method_;
    bool with_reasons_;
    bool use_report_;
};

}  // namespace launchdarkly::config::detail::builders
