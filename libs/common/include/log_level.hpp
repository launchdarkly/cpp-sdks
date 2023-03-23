#pragma once

namespace launchdarkly {
/**
 * Log levels with kDebug being lowest severity and kError being highest
 * severity.
 */
enum class LogLevel {
    kDebug = 0,
    kInfo,
    kWarn,
    kError,
};

char const* GetLDLogLevelName(LogLevel level);
}  // namespace launchdarkl
