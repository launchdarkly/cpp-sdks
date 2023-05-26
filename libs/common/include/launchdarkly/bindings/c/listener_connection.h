// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/export.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

/**
 * Handle that represents a listener connection.
 *
 * To stop unregister a listener call LDListenerConnection_Disconnect.
 * To free a connection listener call LDListenerConnection_Free.
 *
 * Freeing an LDListenerConnection does not disconnect the connection. If it is
 * deleted, without being disconnected, then the listener will remain active
 * until the associated SDK is freed.
*/
typedef struct _LDListenerConnection* LDListenerConnection;

/**
 * Disconnect a listener.
 *
 * @param connection The connection for the listener to disconnect.
 * Must not be NULL.
 */
LD_EXPORT(void) LDListenerConnection_Disconnect(LDListenerConnection connection);

/**
 * Free a listener connection.
 *
 * @param connection The LDListenerConnection to free.
 */
LD_EXPORT(void) LDListenerConnection_Free(LDListenerConnection connection);

#ifdef __cplusplus
}
#endif
