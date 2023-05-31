/** @file */
// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/export.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDStatus* LDStatus;

/**
 * Returns a string representing the error stored in an LDStatus, or
 * NULL if the result indicates success.
 * @param error Result to inspect.
 * @return String or NULL. The returned string is valid only while the LDStatus
 * is alive.
 */
LD_EXPORT(char const*) LDStatus_Error(LDStatus res);

/**
 * Checks if a result indicates success.
 * @param result Result to inspect.
 * @return True if the result indicates success.
 */
LD_EXPORT(bool) LDStatus_Ok(LDStatus res);

/**
 * Frees an LDStatus. It is only necessary to call LDStatus_Free if LDStatus_Ok
 * returns false.
 * @param res Result to free.
 */
LD_EXPORT(void) LDStatus_Free(LDStatus res);

/**
 * Returns a status representing success.
 * @return Successful status.
 */
LD_EXPORT(LDStatus) LDStatus_Success(void);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
