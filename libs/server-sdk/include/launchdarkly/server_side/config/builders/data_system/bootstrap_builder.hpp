#pragma once

#include <launchdarkly/server_side/config/built/data_system/bootstrap_config.hpp>

#include <optional>

namespace launchdarkly::server_side::config::builders {

class BootstrapBuilder {
   public:
    BootstrapBuilder();

    [[nodiscard]] std::optional<built::BootstrapConfig> Build() const;

   private:
    std::optional<built::BootstrapConfig> config_;
};
}  // namespace launchdarkly::server_side::config::builders
