#include <launchdarkly/config/shared/builders/data_system/bootstrap_builder.hpp>

#include "launchdarkly/config/shared/defaults.hpp"

namespace launchdarkly::config::shared::builders {

BootstrapBuilder::BootstrapBuilder()
    : config_(Defaults<ServerSDK>::BootstrapConfig()) {}

std::optional<built::BootstrapConfig> BootstrapBuilder::Build() const {
    return config_;
}
}  // namespace launchdarkly::config::shared::builders
