#include <launchdarkly/bindings/c/logging/log_level.h>

#include <launchdarkly/logging/log_level.hpp>

LD_EXPORT(char const*) LDLogLevel_Name(enum LDLogLevel level) {
    return GetLogLevelName(static_cast<launchdarkly::LogLevel>(level),
                           "unknown");
}
