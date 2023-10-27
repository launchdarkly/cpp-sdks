#include <launchdarkly/config/shared/builders/data_system/data_destination_builder.hpp>

namespace launchdarkly::config::shared::builders {

DataDestinationBuilder<ServerSDK>::DataDestinationBuilder() : config_() {}

[[nodiscard]] config::shared::built::DataDestinationConfig<ServerSDK>
DataDestinationBuilder<ServerSDK>::Build() const {
    return config_;
}

}  // namespace launchdarkly::config::shared::builders
