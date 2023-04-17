#include "config/detail/data_source_builder.hpp"

namespace launchdarkly::config::detail {

StreamingBuilder::StreamingBuilder()
    : config_(Defaults<AnySDK>::StreamingConfig()) {}

StreamingBuilder& StreamingBuilder::initial_reconnect_delay(
    std::chrono::milliseconds initial_reconnect_delay) {
    config_.initial_reconnect_delay = initial_reconnect_delay;
    return *this;
}

StreamingConfig StreamingBuilder::build() const {
    return config_;
}

PollingBuilder::PollingBuilder() : config_(Defaults<AnySDK>::PollingConfig()) {}

PollingBuilder& PollingBuilder::poll_interval(
    std::chrono::seconds poll_interval) {
    config_.poll_interval = poll_interval;
    return *this;
}

PollingConfig PollingBuilder::build() const {
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

DataSourceConfig<ClientSDK> DataSourceBuilder<ClientSDK>::build() const {
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

DataSourceConfig<ServerSDK> DataSourceBuilder<ServerSDK>::build() const {
    auto method = boost::apply_visitor(MethodVisitor(), method_);
    return {method};
}

}  // namespace launchdarkly::config::detail
