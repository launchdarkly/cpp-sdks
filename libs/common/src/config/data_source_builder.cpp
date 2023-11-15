#include <launchdarkly/config/shared/builders/data_source_builder.hpp>

namespace launchdarkly::config::shared::builders {

template <typename SDK>
struct MethodVisitor {};

template <>
struct MethodVisitor<ClientSDK> {
    using SDK = ClientSDK;
    using Result =
        std::variant<built::StreamingConfig<SDK>, built::PollingConfig<SDK>>;

    Result operator()(StreamingBuilder<SDK> const& streaming) const {
        return streaming.Build();
    }

    Result operator()(PollingBuilder<SDK> const& polling) const {
        return polling.Build();
    }
};

template <>
struct MethodVisitor<ServerSDK> {
    using SDK = ServerSDK;
    using Result = tl::expected<std::variant<built::StreamingConfig<SDK>,
                                             built::PollingConfig<SDK>,
                                             built::RedisPullConfig>,
                                Error>;

    Result operator()(StreamingBuilder<SDK> const& streaming) const {
        return streaming.Build();
    }

    Result operator()(PollingBuilder<SDK> const& polling) const {
        return polling.Build();
    }

    Result operator()(RedisPullBuilder const& redis_pull) const {
        return redis_pull.Build().map([](auto&& config) {
            return std::variant<built::StreamingConfig<SDK>,
                                built::PollingConfig<SDK>,
                                built::RedisPullConfig>{std::move(config)};
        });
    }
};

template <typename SDK>
StreamingBuilder<SDK>::StreamingBuilder()
    : config_(Defaults<SDK>::StreamingConfig()) {}

template <typename SDK>
StreamingBuilder<SDK>& StreamingBuilder<SDK>::InitialReconnectDelay(
    std::chrono::milliseconds initial_reconnect_delay) {
    config_.initial_reconnect_delay = initial_reconnect_delay;
    return *this;
}

template <typename SDK>
built::StreamingConfig<SDK> StreamingBuilder<SDK>::Build() const {
    return config_;
}

template <typename SDK>
PollingBuilder<SDK>::PollingBuilder()
    : config_(Defaults<SDK>::PollingConfig()) {}

template <typename SDK>
PollingBuilder<SDK>& PollingBuilder<SDK>::PollInterval(
    std::chrono::seconds poll_interval) {
    config_.poll_interval = poll_interval;
    return *this;
}

template <typename SDK>
built::PollingConfig<SDK> PollingBuilder<SDK>::Build() const {
    return config_;
}

RedisPullBuilder::RedisPullBuilder()
    : config_(Defaults<ServerSDK>::RedisPullConfig()) {}

RedisPullBuilder& RedisPullBuilder::Connection(ConnOpts opts) {
    config_.connection_ = opts;
    return *this;
}

RedisPullBuilder& RedisPullBuilder::Connection(ConnURI uri) {
    config_.connection_ = uri;
    return *this;
}

tl::expected<built::RedisPullConfig, Error> RedisPullBuilder::Build() const {
    if (std::holds_alternative<ConnOpts>(config_.connection_)) {
        auto const& opts = std::get<ConnOpts>(config_.connection_);
        if (opts.host.empty()) {
            return tl::make_unexpected(
                Error::kConfig_DataSource_Redis_EmptyHost);
        }
        if (opts.port == 0) {
            return tl::make_unexpected(
                Error::kConfig_DataSource_Redis_MissingPort);
        }
    } else {
        auto const& uri = std::get<ConnURI>(config_.connection_);
        if (uri.empty()) {
            return tl::make_unexpected(
                Error::kConfig_DataSource_Redis_EmptyURI);
        }
    }
    return config_;
}

DataSourceBuilder<ClientSDK>::DataSourceBuilder()
    : with_reasons_(false), use_report_(false), method_(Streaming()) {}

DataSourceBuilder<ClientSDK>& DataSourceBuilder<ClientSDK>::WithReasons(
    bool value) {
    with_reasons_ = value;
    return *this;
}

DataSourceBuilder<ClientSDK>& DataSourceBuilder<ClientSDK>::UseReport(
    bool value) {
    use_report_ = value;
    return *this;
}

DataSourceBuilder<ClientSDK>& DataSourceBuilder<ClientSDK>::Method(
    StreamingBuilder<ClientSDK> builder) {
    method_ = std::move(builder);
    return *this;
}

DataSourceBuilder<ClientSDK>& DataSourceBuilder<ClientSDK>::Method(
    PollingBuilder<ClientSDK> builder) {
    method_ = std::move(builder);
    return *this;
}

built::DataSourceConfig<ClientSDK> DataSourceBuilder<ClientSDK>::Build() const {
    return {std::visit(MethodVisitor<ClientSDK>(), method_), with_reasons_,
            use_report_};
}

template class PollingBuilder<ClientSDK>;
template class PollingBuilder<ServerSDK>;

template class StreamingBuilder<ClientSDK>;
template class StreamingBuilder<ServerSDK>;

}  // namespace launchdarkly::config::shared::builders
