#include <launchdarkly/config/shared/builders/data_source_builder.hpp>

namespace launchdarkly::config::shared::builders {

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
    method_ = builder;
    return *this;
}

DataSourceBuilder<ClientSDK>& DataSourceBuilder<ClientSDK>::Method(
    PollingBuilder<ClientSDK> builder) {
    method_ = builder;
    return *this;
}

built::DataSourceConfig<ClientSDK> DataSourceBuilder<ClientSDK>::Build() const {
    auto method = std::visit(MethodVisitor<ClientSDK>(), method_);
    return {method, with_reasons_, use_report_};
}

DataSourceBuilder<ServerSDK>::DataSourceBuilder()
    : method_(Streaming()),
      enable_bootstrap_(true),
      enable_sync_(false),
      order_(0) {}

DataSourceBuilder<ServerSDK>& DataSourceBuilder<ServerSDK>::Method(
    StreamingBuilder<ServerSDK> builder) {
    method_ = builder;
    return *this;
}

DataSourceBuilder<ServerSDK>& DataSourceBuilder<ServerSDK>::Method(
    PollingBuilder<ServerSDK> builder) {
    method_ = builder;
    return *this;
}

built::DataSourceConfig<ServerSDK> DataSourceBuilder<ServerSDK>::Build() const {
    auto method = std::visit(MethodVisitor<ServerSDK>(), method_);
    return {method};
}

template class PollingBuilder<config::shared::ClientSDK>;
template class PollingBuilder<config::shared::ServerSDK>;

template class StreamingBuilder<config::shared::ClientSDK>;
template class StreamingBuilder<config::shared::ServerSDK>;

}  // namespace launchdarkly::config::shared::builders
