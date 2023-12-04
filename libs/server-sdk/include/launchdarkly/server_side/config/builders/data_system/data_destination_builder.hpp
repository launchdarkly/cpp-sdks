#pragma once

#include <launchdarkly/server_side/config/built/data_system/data_destination_config.hpp>

namespace launchdarkly::server_side::config::builders {

struct DataDestinationBuilder {
    DataDestinationBuilder();

    [[nodiscard]] built::DataDestinationConfig Build() const;

   private:
    built::DataDestinationConfig config_;
};

}  // namespace launchdarkly::server_side::config::builders
