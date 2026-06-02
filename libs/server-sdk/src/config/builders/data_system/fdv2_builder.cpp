#include <launchdarkly/server_side/config/builders/data_system/fdv2_builder.hpp>

#include "defaults.hpp"

namespace launchdarkly::server_side::config::builders {

FDv2Builder::FDv2Builder() : config_(Defaults::FDv2Config()) {}

FDv2Builder& FDv2Builder::Streaming(StreamingSource source) {
    config_.streaming = source.Build();
    return *this;
}

FDv2Builder& FDv2Builder::Polling(PollingSource source) {
    config_.polling = source.Build();
    return *this;
}

FDv2Builder& FDv2Builder::FDv1Fallback(StreamingSource source) {
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
