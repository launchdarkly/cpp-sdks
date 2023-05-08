// NOLINTBEGIN modernize-use-using

#pragma once

#include "../error.h"
#include "../export.h"

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDClientConfigBuilder* LDClientConfigBuilder;
typedef struct _LDClientConfig* LDClientConfig;

LD_EXPORT(LDClientConfigBuilder) LDClientConfigBuilder_New(char const* sdk_key);

LD_EXPORT(LDError)
LDClientConfigBuilder_Build(LDClientConfigBuilder builder,
                            LDClientConfig* out_config);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
