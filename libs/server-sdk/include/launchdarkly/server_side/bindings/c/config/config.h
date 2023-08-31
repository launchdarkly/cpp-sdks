/** @file */
// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/export.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDCServerConfig* LDServerConfig;

/**
 * Free the configuration. Configurations passed into an LDServerSDK_New call do
 * not need to be freed.
 * @param config Config to free.
 */
LD_EXPORT(void) LDServerConfig_Free(LDServerConfig config);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
