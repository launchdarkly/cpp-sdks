#include <launchdarkly/config/shared/builders/data_source_builder.hpp>

namespace launchdarkly::config::shared::builders {
template <typename SDK>
struct MethodVisitor {
};

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

template <typename SDK>
StreamingBuilder<SDK>::StreamingBuilder()
    : config_(Defaults<SDK>::StreamingConfig()) {
}

template <typename SDK>
StreamingBuilder<SDK>& StreamingBuilder<SDK>::InitialReconnectDelay(
    std::chrono::milliseconds initial_reconnect_delay) {
    config_.initial_reconnect_delay = initial_reconnect_delay;
    return *this;
}

template <typename SDK>
StreamingBuilder<SDK>& StreamingBuilder<SDK>::Filter(std::string filter_key) {
    config_.filter_key = std::move(filter_key);
    return *this;
}

template <typename SDK>
built::StreamingConfig<SDK> StreamingBuilder<SDK>::Build() const {
    return config_;
}

template <typename SDK>
PollingBuilder<SDK>::PollingBuilder()
    : config_(Defaults<SDK>::PollingConfig()) {
}

template <typename SDK>
PollingBuilder<SDK>& PollingBuilder<SDK>::PollInterval(
    std::chrono::seconds poll_interval) {
    config_.poll_interval = poll_interval;
    return *this;
}

template <typename SDK>
PollingBuilder<SDK>& PollingBuilder<SDK>::Filter(std::string filter_key) {
    config_.filter_key = std::move(filter_key);
    return *this;
}

template <typename SDK>
built::PollingConfig<SDK> PollingBuilder<SDK>::Build() const {
    return config_;
}

DataSourceBuilder<ClientSDK>::DataSourceBuilder()
    : with_reasons_(false), use_report_(false), method_(Streaming()) {
}

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
} // namespace launchdarkly::config::shared::builders
