#include "config/detail/builders/data_source_builder.hpp"

namespace launchdarkly::config::detail::builders {

StreamingBuilder::StreamingBuilder()
    : config_(Defaults<AnySDK>::StreamingConfig()) {}

StreamingBuilder& StreamingBuilder::InitialReconnectDelay(
    std::chrono::milliseconds initial_reconnect_delay) {
    config_.initial_reconnect_delay = initial_reconnect_delay;
    return *this;
}

built::StreamingConfig StreamingBuilder::Build() const {
    return config_;
}

PollingBuilder::PollingBuilder() : config_(Defaults<AnySDK>::PollingConfig()) {}

PollingBuilder& PollingBuilder::poll_interval(
    std::chrono::seconds poll_interval) {
    config_.poll_interval = poll_interval;
    return *this;
}

built::PollingConfig PollingBuilder::Build() const {
    return config_;
}

DataSourceBuilder<ClientSDK>::DataSourceBuilder()
    : with_reasons_(false), use_report_(false), method_(Streaming()) {}

DataSourceBuilder<ClientSDK>& DataSourceBuilder<ClientSDK>::with_reasons(
    bool value) {
    with_reasons_ = value;
    return *this;
}

DataSourceBuilder<ClientSDK>& DataSourceBuilder<ClientSDK>::use_report(
    bool value) {
    use_report_ = value;
    return *this;
}

DataSourceBuilder<ClientSDK>& DataSourceBuilder<ClientSDK>::method(
    StreamingBuilder builder) {
    method_ = builder;
    return *this;
}

DataSourceBuilder<ClientSDK>& DataSourceBuilder<ClientSDK>::method(
    PollingBuilder builder) {
    method_ = builder;
    return *this;
}

built::DataSourceConfig<ClientSDK> DataSourceBuilder<ClientSDK>::Build() const {
    auto method = boost::apply_visitor(MethodVisitor(), method_);
    return {method, with_reasons_, use_report_};
}

DataSourceBuilder<ServerSDK>::DataSourceBuilder()
    : with_reasons_(false), use_report_(false), method_(Streaming()) {}

DataSourceBuilder<ServerSDK>& DataSourceBuilder<ServerSDK>::method(
    StreamingBuilder builder) {
    method_ = builder;
    return *this;
}

DataSourceBuilder<ServerSDK>& DataSourceBuilder<ServerSDK>::method(
    PollingBuilder builder) {
    method_ = builder;
    return *this;
}

built::DataSourceConfig<ServerSDK> DataSourceBuilder<ServerSDK>::Build() const {
    auto method = boost::apply_visitor(MethodVisitor(), method_);
    return {method};
}

}  // namespace launchdarkly::config::detail::builders
