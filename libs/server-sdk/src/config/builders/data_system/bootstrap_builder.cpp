#include <launchdarkly/server_side/config/builders/data_system/bootstrap_builder.hpp>

#include "defaults.hpp"

namespace launchdarkly::server_side::config::builders {

BootstrapBuilder::BootstrapBuilder() : config_(Defaults::BootstrapConfig()) {}

std::optional<built::BootstrapConfig> BootstrapBuilder::Build() const {
    return config_;
}
}  // namespace launchdarkly::server_side::config::builders
