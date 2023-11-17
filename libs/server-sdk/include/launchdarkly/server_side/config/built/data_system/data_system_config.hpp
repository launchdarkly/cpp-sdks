#pragma once

#include <launchdarkly/server_side/config/built/data_system/background_sync_config.hpp>
#include <launchdarkly/server_side/config/built/data_system/lazy_load_config.hpp>

#include <variant>

namespace launchdarkly::server_side::config::built {

struct DataSystemConfig {
    bool disabled;
    std::variant<LazyLoadConfig, BackgroundSyncConfig> system_;
};

}  // namespace launchdarkly::server_side::config::built
