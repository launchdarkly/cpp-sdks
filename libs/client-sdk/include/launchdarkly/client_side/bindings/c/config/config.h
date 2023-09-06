/** @file */
// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/export.h>
#include <launchdarkly/bindings/c/status.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDClientConfig* LDClientConfig;

/**
 * Free an unused configuration. Configurations used to construct an LDClientSDK
 * must not be be freed.
 *
 * @param config Config to free.
 */
LD_EXPORT(void) LDClientConfig_Free(LDClientConfig config);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
