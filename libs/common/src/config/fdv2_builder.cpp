#include <launchdarkly/config/shared/builders/fdv2_builder.hpp>
#include <launchdarkly/config/shared/defaults.hpp>

#include <utility>

namespace launchdarkly::config::shared::builders {

FDv2Builder::Config::CacheConfig FDv2Builder::Cache::Build() const {
    return {};
}

FDv2Builder::Streaming& FDv2Builder::Streaming::InitialReconnectDelay(
    std::chrono::milliseconds delay) {
    initial_reconnect_delay_ = delay;
    return *this;
}

FDv2Builder::Streaming& FDv2Builder::Streaming::BaseUrl(std::string base_url) {
    base_url_override_ = std::move(base_url);
    return *this;
}

FDv2Builder::Config::StreamingConfig FDv2Builder::Streaming::Build() const {
    return {initial_reconnect_delay_, base_url_override_};
}

FDv2Builder::Polling& FDv2Builder::Polling::PollInterval(
    std::chrono::seconds interval) {
    poll_interval_ = interval;
    return *this;
}

FDv2Builder::Polling& FDv2Builder::Polling::BaseUrl(std::string base_url) {
    base_url_override_ = std::move(base_url);
    return *this;
}

FDv2Builder::Config::PollingConfig FDv2Builder::Polling::Build() const {
    return {poll_interval_, base_url_override_};
}

FDv2Builder::FDv1Fallback& FDv2Builder::FDv1Fallback::PollInterval(
    std::chrono::seconds interval) {
    poll_interval_ = interval;
    return *this;
}

FDv2Builder::FDv1Fallback& FDv2Builder::FDv1Fallback::BaseUrl(
    std::string base_url) {
    base_url_override_ = std::move(base_url);
    return *this;
}

FDv2Builder::Config::FDv1FallbackConfig FDv2Builder::FDv1Fallback::Build()
    const {
    return {poll_interval_, base_url_override_};
}

FDv2Builder::Mode& FDv2Builder::Mode::Initializer(Cache source) {
    definition_.initializers.emplace_back(source.Build());
    return *this;
}

FDv2Builder::Mode& FDv2Builder::Mode::Initializer(Polling source) {
    definition_.initializers.emplace_back(source.Build());
    return *this;
}

FDv2Builder::Mode& FDv2Builder::Mode::Synchronizer(Streaming source) {
    definition_.synchronizers.emplace_back(source.Build());
    return *this;
}

FDv2Builder::Mode& FDv2Builder::Mode::Synchronizer(Polling source) {
    definition_.synchronizers.emplace_back(source.Build());
    return *this;
}

FDv2Builder::Mode& FDv2Builder::Mode::FallbackToFDv1(FDv1Fallback source) {
    definition_.fdv1_fallback = source.Build();
    return *this;
}

FDv2Builder::Mode& FDv2Builder::Mode::DisableFDv1Fallback() {
    definition_.fdv1_fallback = std::nullopt;
    return *this;
}

FDv2Builder::Config::ModeDefinition FDv2Builder::Mode::Build() const {
    return definition_;
}

FDv2Builder::FDv2Builder() : config_(Defaults<ClientSDK>::FDv2Config()) {}

FDv2Builder& FDv2Builder::InitialMode(ConnectionMode mode) {
    config_.initial_mode = mode;
    return *this;
}

FDv2Builder& FDv2Builder::CustomizeMode(ConnectionMode mode, Mode definition) {
    config_.modes[mode] = definition.Build();
    return *this;
}

FDv2Builder& FDv2Builder::UsePost(bool use_post) {
    config_.use_post = use_post;
    return *this;
}

FDv2Builder::Config FDv2Builder::Build() const {
    return config_;
}

}  // namespace launchdarkly::config::shared::builders
