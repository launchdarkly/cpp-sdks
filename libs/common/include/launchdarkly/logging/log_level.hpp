#pragma once

#include <cstdint>
#include <ostream>

namespace launchdarkly {
/**
 * Log levels with kDebug being lowest severity and kError being highest
 * severity. The values must not be changed to ensure backwards compatibility
 * with the C API.
 */
enum class LogLevel : std::uint32_t {
    kDebug = 0,
    kInfo = 1,
    kWarn = 2,
    kError = 3,
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

std::ostream& operator<<(std::ostream& out, LogLevel const& level);

}  // namespace launchdarkly
