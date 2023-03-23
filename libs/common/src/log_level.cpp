#include "log_level.hpp"

namespace launchdarkly {
char const* GetLDLogLevelName(LogLevel level) {
    switch (level) {
        case LogLevel::kDebug: {
            return "kDebug";
        } break;
        case LogLevel::kInfo: {
            return "kInfo";
        } break;
        case LogLevel::kWarn: {
            return "kWarn";
        } break;
        case LogLevel::kError: {
            return "kError";
        } break;
    }
    return "INVALID";
}
}  // namespace launchdarkly
