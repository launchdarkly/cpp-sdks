#include <launchdarkly/server_side/config/builders/data_system/fdv2_builder.hpp>

#include "defaults.hpp"

namespace launchdarkly::server_side::config::builders {

FDv2Builder::Streaming& FDv2Builder::Streaming::InitialReconnectDelay(
    std::chrono::milliseconds delay) {
    initial_reconnect_delay_ = delay;
    return *this;
}

FDv2Builder::Streaming& FDv2Builder::Streaming::Filter(std::string filter_key) {
    filter_key_ = std::move(filter_key);
    return *this;
}

FDv2Builder::Streaming& FDv2Builder::Streaming::BaseUrl(std::string base_url) {
    base_url_override_ = std::move(base_url);
    return *this;
}

built::FDv2Config::StreamingConfig FDv2Builder::Streaming::Build() const {
    return {initial_reconnect_delay_, filter_key_, base_url_override_};
}

FDv2Builder::Polling& FDv2Builder::Polling::PollInterval(
    std::chrono::seconds interval) {
    poll_interval_ = interval;
    return *this;
}

FDv2Builder::Polling& FDv2Builder::Polling::Filter(std::string filter_key) {
    filter_key_ = std::move(filter_key);
    return *this;
}

FDv2Builder::Polling& FDv2Builder::Polling::BaseUrl(std::string base_url) {
    base_url_override_ = std::move(base_url);
    return *this;
}

built::FDv2Config::PollingConfig FDv2Builder::Polling::Build() const {
    return {poll_interval_, filter_key_, base_url_override_};
}

FDv2Builder::FDv2Builder()
    : config_{{},
              {},
              std::nullopt,
              std::chrono::minutes{2},
              std::chrono::minutes{5}} {}

FDv2Builder FDv2Builder::Default() {
    return FDv2Builder()
        .Initializer(Polling{})
        .Synchronizer(Streaming{})
        .Synchronizer(Polling{})
        .FDv1Fallback(FDv1Streaming{});
}

FDv2Builder& FDv2Builder::Initializer(Polling source) {
    config_.initializers.push_back(source.Build());
    return *this;
}

FDv2Builder& FDv2Builder::Synchronizer(Streaming source) {
    config_.synchronizers.push_back(source.Build());
    return *this;
}

FDv2Builder& FDv2Builder::Synchronizer(Polling source) {
    config_.synchronizers.push_back(source.Build());
    return *this;
}

FDv2Builder& FDv2Builder::FDv1Fallback(FDv1Streaming source) {
    config_.fdv1_fallback = source.Build();
    return *this;
}

FDv2Builder& FDv2Builder::FDv1Fallback(FDv1Polling source) {
    config_.fdv1_fallback = source.Build();
    return *this;
}

FDv2Builder& FDv2Builder::DisableFDv1Fallback() {
    config_.fdv1_fallback = std::nullopt;
    return *this;
}

FDv2Builder& FDv2Builder::FallbackTimeout(std::chrono::milliseconds timeout) {
    config_.fallback_timeout = timeout;
    return *this;
}

FDv2Builder& FDv2Builder::RecoveryTimeout(std::chrono::milliseconds timeout) {
    config_.recovery_timeout = timeout;
    return *this;
}

built::FDv2Config FDv2Builder::Build() const {
    return config_;
}

}  // namespace launchdarkly::server_side::config::builders
