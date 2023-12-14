/** @file */
// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/export.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
// only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDStatus* LDStatus;

/**
 * Returns a string describing the error reported by the LDStatus, or
 * NULL if the status indicates success.
 * @param status Status to inspect.
 * @return String or NULL. The string is valid only while the LDStatus
 * is alive.
 */
LD_EXPORT(char const*) LDStatus_Error(LDStatus status);

/**
 * Checks if the status indicates successful operation.
 * @param status Status to inspect.
 * @return True if the status indicates success.
 */
LD_EXPORT(bool) LDStatus_Ok(LDStatus status);

/**
 * Frees an LDStatus. You must call LDStatus_Free if LDStatus_Ok
 * returns false.
 * @param status Status to free.
 */
LD_EXPORT(void) LDStatus_Free(LDStatus status);

/**
 * Creates a status representing successful operation.
 * @return Successful status.
 */
LD_EXPORT(LDStatus) LDStatus_Success(void);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
