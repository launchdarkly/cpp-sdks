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

LD_EXPORT(char const*) LDLogLevel_Name(enum LDLogLevel level);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
