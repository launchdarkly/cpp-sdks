#pragma once

#include <launchdarkly/config/shared/defaults.hpp>
#include <launchdarkly/config/shared/sdks.hpp>

#include <chrono>
#include <optional>
#include <type_traits>
#include <variant>

namespace launchdarkly::config::shared::builders {

class BootstrapBuilder {
   public:
    class DefaultBuilder {};

    BootstrapBuilder();

    BootstrapBuilder& Default();

    [[nodiscard]] built::BootstrapConfig Build() const;

   private:
    using BootstrapType = std::variant<DefaultBuilder>;
    BootstrapType bootstrapper_;
};
}  // namespace launchdarkly::config::shared::builders
