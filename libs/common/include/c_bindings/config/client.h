// NOLINTBEGIN modernize-use-using

#pragma once

#include "../export.h"
#include "../status.h"

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDClientConfigBuilder* LDClientConfigBuilder;
typedef struct _LDClientConfig* LDClientConfig;

LD_EXPORT(LDClientConfigBuilder) LDClientConfigBuilder_New(char const* sdk_key);

/**
 * Creates an LDClientConfig. The LDClientConfigBuilder is consumed.
 * On success, the config will be stored in out_config. Otherwise,
 * out_config will be set to NULL and the return value will contain the error.
 * @param builder Builder to consume.
 * @param out_config Pointer to where the built config will be
 * stored.
 * @return Error result on failure.
 */
LD_EXPORT(LDStatus)
LDClientConfigBuilder_Build(LDClientConfigBuilder builder,
                            LDClientConfig* out_config);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
