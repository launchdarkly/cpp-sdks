#include <launchdarkly/server_side/config/builders/data_system/data_destination_builder.hpp>

namespace launchdarkly::server_side::config::builders {

DataDestinationBuilder::DataDestinationBuilder() : config_() {}

[[nodiscard]] built::DataDestinationConfig DataDestinationBuilder::Build()
    const {
    return config_;
}

}  // namespace launchdarkly::server_side::config::builders
