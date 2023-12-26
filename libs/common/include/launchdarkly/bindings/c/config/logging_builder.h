/** @file */
// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/logging/log_level.h>

#include <launchdarkly/bindings/c/export.h>
#include <launchdarkly/bindings/c/status.h>

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDLoggingBasicBuilder* LDLoggingBasicBuilder;
typedef struct _LDLoggingCustomBuilder* LDLoggingCustomBuilder;

typedef bool (*EnabledFn)(enum LDLogLevel level, void* user_data);
typedef void (*WriteFn)(enum LDLogLevel level,
                        char const* msg,
                        void* user_data);

/**
 * Defines a logging interface suitable for use with SDK configuration.
 */
struct LDLogBackend {
    /**
     * Check if the specified log level is enabled. Must be thread safe.
     * @param level The log level to check.
     * @return Returns true if the level is enabled.
     */
    EnabledFn Enabled;

    /**
     * Write a message to the specified level. The message pointer is valid only
     * for the duration of this function call. Must be thread safe.
     * @param level The level to write the message to.
     * @param message The message to write.
     */
    WriteFn Write;

    /**
     * UserData is forwarded into both Enabled and Write.
     */
    void* UserData;
};

/**
 * Initializes a custom log backend. Must be called before passing a custom
 * backend into configuration.
 * @param backend Backend to initialize.
 */
LD_EXPORT(void)
LDLogBackend_Init(struct LDLogBackend* backend);

/**
 * Creates a new builder for LaunchDarkly's default logger.
 *
 * If not passed into the config
 * builder, must be manually freed with LDLoggingBasicBuilder_Free.
 * @return New builder.
 */
LD_EXPORT(LDLoggingBasicBuilder)
LDLoggingBasicBuilder_New();

/**
 * Frees a basic logging builder. Do not call if the builder was consumed by
 * the config builder.
 * @param b Builder to free.
 */
LD_EXPORT(void)
LDLoggingBasicBuilder_Free(LDLoggingBasicBuilder b);

/**
 * Sets the enabled log level. The default level is LD_LOG_INFO.
 * @param b Client config builder. Must not be NULL.
 * @param level Level to set.
 */
LD_EXPORT(void)
LDLoggingBasicBuilder_Level(LDLoggingBasicBuilder b, enum LDLogLevel level);

/**
 * Set a tag for this logger. This tag will be included at the start
 * of log entries in square brackets.
 *
 * If the name was "LaunchDarkly", then log entries will be prefixed
 * with "[LaunchDarkly]". The default tag is "LaunchDarkly".
 * @param b Client config builder. Must not be NULL.
 * @param tag Tag to set. Must not be NULL.
 */
LD_EXPORT(void)
LDLoggingBasicBuilder_Tag(LDLoggingBasicBuilder b, char const* tag);

/**
 * Creates a new builder for a custom, user-provided logger.
 *
 * If not passed into the config
 * builder, must be manually freed with LDLoggingCustomBuilder_Free.
 * @return New builder.
 */
LD_EXPORT(LDLoggingCustomBuilder)
LDLoggingCustomBuilder_New();

/**
 * Frees a custom logging builder. Do not call if the builder was consumed by
 * the config builder.
 * @param b Builder to free.
 */
LD_EXPORT(void)
LDLoggingCustomBuilder_Free(LDLoggingCustomBuilder b);

/**
 * Sets a custom log backend.
 * @param b Custom logging builder. Must not be NULL.
 * @param backend The backend to use for logging. Ensure the backend was
 * initialized with LDLogBackend_Init.
 */
LD_EXPORT(void)
LDLoggingCustomBuilder_Backend(LDLoggingCustomBuilder b,
                               struct LDLogBackend backend);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
