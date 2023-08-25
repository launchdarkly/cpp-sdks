// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include <launchdarkly/client_side/bindings/c/config/config.h>
#include <launchdarkly/config/client.hpp>

#define TO_CONFIG(ptr) (reinterpret_cast<Config*>(ptr))
#define FROM_CONFIG(ptr) (reinterpret_cast<LDClientConfig>(ptr))

using namespace launchdarkly::client_side;

LD_EXPORT(void) LDClientConfig_Free(LDClientConfig config) {
    delete TO_CONFIG(config);
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
