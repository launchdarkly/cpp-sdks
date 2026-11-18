#include <launchdarkly/config/shared/builders/fdv2_builder.hpp>
#include <launchdarkly/config/shared/defaults.hpp>

#include <utility>

namespace launchdarkly::config::shared::builders {

FDv2Builder<ClientSDK>::Config::CacheConfig
FDv2Builder<ClientSDK>::Cache::Build() const {
    return {};
}

FDv2Builder<ClientSDK>::Streaming&
FDv2Builder<ClientSDK>::Streaming::InitialReconnectDelay(
    std::chrono::milliseconds delay) {
    initial_reconnect_delay_ = delay;
    return *this;
}

FDv2Builder<ClientSDK>::Streaming& FDv2Builder<ClientSDK>::Streaming::BaseUrl(
    std::string base_url) {
    base_url_override_ = std::move(base_url);
    return *this;
}

FDv2Builder<ClientSDK>::Config::StreamingConfig
FDv2Builder<ClientSDK>::Streaming::Build() const {
    return {initial_reconnect_delay_, base_url_override_};
}

FDv2Builder<ClientSDK>::Polling& FDv2Builder<ClientSDK>::Polling::PollInterval(
    std::chrono::seconds interval) {
    poll_interval_ = interval;
    return *this;
}

FDv2Builder<ClientSDK>::Polling& FDv2Builder<ClientSDK>::Polling::BaseUrl(
    std::string base_url) {
    base_url_override_ = std::move(base_url);
    return *this;
}

FDv2Builder<ClientSDK>::Config::PollingConfig
FDv2Builder<ClientSDK>::Polling::Build() const {
    return {poll_interval_, base_url_override_};
}

FDv2Builder<ClientSDK>::FDv1Polling& FDv2Builder<
    ClientSDK>::FDv1Polling::PollInterval(std::chrono::seconds interval) {
    poll_interval_ = interval;
    return *this;
}

FDv2Builder<ClientSDK>::FDv1Polling&
FDv2Builder<ClientSDK>::FDv1Polling::BaseUrl(std::string base_url) {
    base_url_override_ = std::move(base_url);
    return *this;
}

FDv2Builder<ClientSDK>::Config::FDv1FallbackConfig
FDv2Builder<ClientSDK>::FDv1Polling::Build() const {
    return {poll_interval_, base_url_override_};
}

FDv2Builder<ClientSDK>::Mode& FDv2Builder<ClientSDK>::Mode::Initializer(
    Cache source) {
    definition_.initializers.emplace_back(source.Build());
    return *this;
}

FDv2Builder<ClientSDK>::Mode& FDv2Builder<ClientSDK>::Mode::Initializer(
    Polling source) {
    definition_.initializers.emplace_back(source.Build());
    return *this;
}

FDv2Builder<ClientSDK>::Mode& FDv2Builder<ClientSDK>::Mode::Synchronizer(
    Streaming source) {
    definition_.synchronizers.emplace_back(source.Build());
    return *this;
}

FDv2Builder<ClientSDK>::Mode& FDv2Builder<ClientSDK>::Mode::Synchronizer(
    Polling source) {
    definition_.synchronizers.emplace_back(source.Build());
    return *this;
}

FDv2Builder<ClientSDK>::Mode& FDv2Builder<ClientSDK>::Mode::FDv1Fallback(
    FDv1Polling source) {
    definition_.fdv1_fallback = source.Build();
    return *this;
}

FDv2Builder<ClientSDK>::Mode&
FDv2Builder<ClientSDK>::Mode::DisableFDv1Fallback() {
    definition_.fdv1_fallback = Config::NoFDv1Fallback{};
    return *this;
}

FDv2Builder<ClientSDK>::Config::ModeDefinition
FDv2Builder<ClientSDK>::Mode::Build() const {
    return definition_;
}

FDv2Builder<ClientSDK>::FDv2Builder()
    : config_(Defaults<ClientSDK>::FDv2Config()) {}

FDv2Builder<ClientSDK>& FDv2Builder<ClientSDK>::InitialMode(
    ConnectionMode mode) {
    config_.initial_mode = mode;
    return *this;
}

FDv2Builder<ClientSDK>& FDv2Builder<ClientSDK>::CustomizeMode(
    ConnectionMode mode,
    Mode definition) {
    config_.modes[mode] = definition.Build();
    return *this;
}

FDv2Builder<ClientSDK>& FDv2Builder<ClientSDK>::UsePost(bool use_post) {
    config_.use_post = use_post;
    return *this;
}

FDv2Builder<ClientSDK>::Config FDv2Builder<ClientSDK>::Build() const {
    return config_;
}

}  // namespace launchdarkly::config::shared::builders
