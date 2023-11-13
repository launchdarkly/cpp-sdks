#pragma once

#include <launchdarkly/config/shared/built/data_system/bootstrap_config.hpp>

#include <optional>

namespace launchdarkly::config::shared::builders {

class BootstrapBuilder {
   public:
    BootstrapBuilder();

    [[nodiscard]] std::optional<built::BootstrapConfig> Build() const;

   private:
    std::optional<built::BootstrapConfig> config_;
};
}  // namespace launchdarkly::config::shared::builders
