/** @file */
// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/export.h>
#include <launchdarkly/bindings/c/status.h>

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

/**
 * Defines the log levels used with the SDK's default logger, or a user-provided
 * custom logger.
 */
enum LDLogLevel {
    LD_LOG_DEBUG = 0,
    LD_LOG_INFO = 1,
    LD_LOG_WARN = 2,
    LD_LOG_ERROR = 3,
};

/**
 * Lookup the name of a LDLogLevel.
 * @param level Target level.
 * @param level_if_unknown Default name to return if the level wasn't
 * recognized.
 * @return Name of the level as a string, or level_if_unknown if not recognized.
 */
LD_EXPORT(char const*)
LDLogLevel_Name(enum LDLogLevel level, char const* level_if_unknown);

/**
 * Lookup a LDLogLevel by name.
 * @param level Name of level.
 * @param level_if_unknown Default level to return if the level wasn't
 * recognized.
 * @return LDLogLevel matching the name, or level_if_unknown if not recognized.
 */
LD_EXPORT(enum LDLogLevel)
LDLogLevel_Enum(char const* level, enum LDLogLevel level_if_unknown);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
