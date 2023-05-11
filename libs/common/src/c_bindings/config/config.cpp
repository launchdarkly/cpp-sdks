// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include "c_bindings/config/config.h"
#include "config/client.hpp"

#define TO_CONFIG(ptr) (reinterpret_cast<Config*>(ptr))
#define FROM_CONFIG(ptr) (reinterpret_cast<LDClientConfig>(ptr))

using namespace launchdarkly::client_side;

LD_EXPORT(void) LDClientConfig_Free(LDClientConfig config) {
    if (Config* c = TO_CONFIG(config)) {
        delete c;
    }
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
