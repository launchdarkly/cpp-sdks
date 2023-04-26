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
template <typename SDK>
class PollingBuilder {
   public:
    PollingBuilder();

    /**
     * Sets the interval at which the SDK will poll for feature flag updates.
     * @param poll_interval The polling interval.
     * @return Reference to this builder.
     */
    PollingBuilder& PollInterval(std::chrono::seconds poll_interval);

    /**
     * Build the polling config. Used internal to the SDK.
     * @return The built config.
     */
    [[nodiscard]] built::PollingConfig<SDK> Build() const;

   private:
    built::PollingConfig<SDK> config_;
};

/**
 * The method visitor is only needed inside this file
 */
namespace {
template <typename SDK>
struct MethodVisitor {
    boost::variant<built::StreamingConfig, built::PollingConfig<SDK>> operator()(
        StreamingBuilder streaming) {
        return streaming.Build();
    }
    boost::variant<built::StreamingConfig, built::PollingConfig<SDK>> operator()(
        PollingBuilder<SDK> polling) {
        return polling.Build();
    }
};
}  // namespace

template <>
class DataSourceBuilder<ClientSDK> {
   public:
    using Streaming = StreamingBuilder;
    using Polling = PollingBuilder<ClientSDK>;

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
    DataSourceBuilder& WithReasons(bool value);

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
    DataSourceBuilder& UseReport(bool value);

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
    DataSourceBuilder& Method(Streaming stream_builder);

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
    DataSourceBuilder& Method(Polling polling_builder);

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
    using Polling = PollingBuilder<ServerSDK>;

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
    DataSourceBuilder& Method(Streaming builder);

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
    DataSourceBuilder& Method(Polling builder);

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
