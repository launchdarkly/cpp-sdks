#include <launchdarkly/config/shared/connection_mode.hpp>

namespace launchdarkly::config::shared {

char const* GetConnectionModeName(ConnectionMode mode) {
    switch (mode) {
        case ConnectionMode::kStreaming:
            return "streaming";
        case ConnectionMode::kPolling:
            return "polling";
        case ConnectionMode::kOffline:
            return "offline";
    }
    return "unknown";
}

}  // namespace launchdarkly::config::shared
