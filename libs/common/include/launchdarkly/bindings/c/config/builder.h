// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/config/config.h>
#include <launchdarkly/bindings/c/export.h>
#include <launchdarkly/bindings/c/status.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDClientConfigBuilder* LDClientConfigBuilder;

LD_EXPORT(LDClientConfigBuilder) LDClientConfigBuilder_New(char const* sdk_key);

/**
 * Creates an LDClientConfig. The LDClientConfigBuilder is consumed.
 * On success, the config will be stored in out_config; otherwise,
 * out_config will be set to NULL and the returned LDStatus will indicate the
 * error.
 * @param builder Builder to consume.
 * @param out_config Pointer to where the built config will be
 * stored.
 * @return Error status on failure.
 */
LD_EXPORT(LDStatus)
LDClientConfigBuilder_Build(LDClientConfigBuilder builder,
                            LDClientConfig* out_config);

/**
 * Frees the builder; only necessary if not calling Build.
 * @param builder Builder to free.
 */
LD_EXPORT(void)
LDClientConfigBuilder_Free(LDClientConfigBuilder builder);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
