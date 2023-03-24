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

/**
 * Lookup the name of a LogLevel.
 * @param level Target level.
 * @param default_ Default name to return if the level wasn't recognized.
 * @return Name of the level as a string, or default_ if not recognized.
 */
char const* GetLogLevelName(LogLevel level, char const* default_);
/**
 * Lookup a LogLevel by name.
 * @param level Name of level.
 * @param default_ Default level to return if the level wasn't recognized.
 * @return LogLevel matching the name, or default_ if not recognized.
 */
LogLevel GetLogLevelEnum(char const* name, LogLevel default_);

}  // namespace launchdarkly
