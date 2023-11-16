// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include <launchdarkly/server_side/bindings/c/config/config.h>
#include <launchdarkly/server_side/config/config.hpp>

#define TO_CONFIG(ptr) (reinterpret_cast<Config*>(ptr))

using namespace launchdarkly::server_side;

LD_EXPORT(void) LDServerConfig_Free(LDServerConfig config) {
    delete TO_CONFIG(config);
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
