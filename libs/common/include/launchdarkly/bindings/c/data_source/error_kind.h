/** @file error_kind.h
 * @brief LaunchDarkly Server-side C Bindings for Data Source Error Kinds.
 */
// NOLINTBEGIN modernize-use-using
#pragma once

#include <launchdarkly/bindings/c/export.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

/**
 * A description of an error condition that the data source encountered.
 */
enum LDDataSourceStatus_ErrorKind {
    /**
     * An unexpected error, such as an uncaught exception, further
     * described by the error message.
     */
    LD_DATASOURCESTATUS_ERRORKIND_UNKNOWN = 0,

    /**
     * An I/O error such as a dropped connection.
     */
    LD_DATASOURCESTATUS_ERRORKIND_NETWORK_ERROR = 1,

    /**
     * The LaunchDarkly service returned an HTTP response with an error
     * status, available in the status code.
     */
    LD_DATASOURCESTATUS_ERRORKIND_ERROR_RESPONSE = 2,

    /**
     * The SDK received malformed data from the LaunchDarkly service.
     */
    LD_DATASOURCESTATUS_ERRORKIND_INVALID_DATA = 3,

    /**
     * The data source itself is working, but when it tried to put an
     * update into the data store, the data store failed (so the SDK may
     * not have the latest data).
     */
    LD_DATASOURCESTATUS_ERRORKIND_STORE_ERROR = 4,
};

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
