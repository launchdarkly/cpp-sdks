#include <launchdarkly/bindings/c/logging/log_level.h>

#include <launchdarkly/logging/log_level.hpp>

LD_EXPORT(char const*)
LDLogLevel_Name(LDLogLevel level, char const* name_if_unknown) {
    return GetLogLevelName(static_cast<launchdarkly::LogLevel>(level),
                           name_if_unknown);
}

LD_EXPORT(LDLogLevel)
LDLogLevel_Enum(char const* level, LDLogLevel level_if_unknown) {
    return static_cast<LDLogLevel>(GetLogLevelEnum(
        level, static_cast<launchdarkly::LogLevel>(level_if_unknown)));
}
