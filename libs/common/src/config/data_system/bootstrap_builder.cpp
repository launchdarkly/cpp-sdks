#include <launchdarkly/config/shared/builders/data_system/bootstrap_builder.hpp>

namespace launchdarkly::config::shared::builders {

BootstrapBuilder::BootstrapBuilder() : bootstrapper_() {}

BootstrapBuilder& BootstrapBuilder::Default() {
    bootstrapper_ = DefaultBuilder();
    return *this;
}

built::BootstrapConfig BootstrapBuilder::Build() const {
    return {
        // TODO
    };
}
}  // namespace launchdarkly::config::shared::builders
