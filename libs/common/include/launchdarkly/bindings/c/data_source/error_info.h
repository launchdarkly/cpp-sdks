/** @file error_info.h
 * @brief LaunchDarkly Server-side C Bindings for Data Source Error Info.
 */
// NOLINTBEGIN modernize-use-using
#pragma once

#include <launchdarkly/bindings/c/data_source/error_kind.h>
#include <launchdarkly/bindings/c/export.h>

#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDDataSourceStatus_ErrorInfo* LDDataSourceStatus_ErrorInfo;

/**
 * Get an enumerated value representing the general category of the error.
 */
LD_EXPORT(enum LDDataSourceStatus_ErrorKind)
LDDataSourceStatus_ErrorInfo_GetKind(LDDataSourceStatus_ErrorInfo info);

/**
 * The HTTP status code if the error was
 * LD_DATASOURCESTATUS_ERRORKIND_ERROR_RESPONSE.
 */
LD_EXPORT(uint64_t)
LDDataSourceStatus_ErrorInfo_StatusCode(LDDataSourceStatus_ErrorInfo info);

/**
 * Any additional human-readable information relevant to the error.
 *
 * The format is subject to change and should not be relied on
 * programmatically.
 */
LD_EXPORT(char const*)
LDDataSourceStatus_ErrorInfo_Message(LDDataSourceStatus_ErrorInfo info);

/**
 * The date/time that the error occurred, in seconds since epoch.
 */
LD_EXPORT(time_t)
LDDataSourceStatus_ErrorInfo_Time(LDDataSourceStatus_ErrorInfo info);

/**
 * Frees the data source status error information.
 * @param status The error information to free.
 */
LD_EXPORT(void)
LDDataSourceStatus_ErrorInfo_Free(LDDataSourceStatus_ErrorInfo info);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
