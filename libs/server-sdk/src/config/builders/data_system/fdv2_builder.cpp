#include <launchdarkly/server_side/config/builders/data_system/fdv2_builder.hpp>

#include "defaults.hpp"

namespace launchdarkly::server_side::config::builders {

FDv2Builder::FDv2Builder()
    : config_(Defaults::FDv2Config()),
      initializers_explicit_(false),
      synchronizers_explicit_(false) {}

FDv2Builder& FDv2Builder::Initializer(Polling source) {
    if (!initializers_explicit_) {
        config_.initializers.clear();
        initializers_explicit_ = true;
    }
    config_.initializers.push_back(source.Build());
    return *this;
}

FDv2Builder& FDv2Builder::Synchronizer(Streaming source) {
    if (!synchronizers_explicit_) {
        config_.synchronizers.clear();
        synchronizers_explicit_ = true;
    }
    config_.synchronizers.push_back(source.Build());
    return *this;
}

FDv2Builder& FDv2Builder::Synchronizer(Polling source) {
    if (!synchronizers_explicit_) {
        config_.synchronizers.clear();
        synchronizers_explicit_ = true;
    }
    config_.synchronizers.push_back(source.Build());
    return *this;
}

FDv2Builder& FDv2Builder::FDv1Fallback(Streaming source) {
    config_.fdv1_fallback = source.Build();
    return *this;
}

FDv2Builder& FDv2Builder::FDv1Fallback(Polling source) {
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
