// NOLINTBEGIN modernize-use-using

#pragma once

#include "../export.h"
#include "../value.h"

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDClientConfigBuilder* LDClientConfigBuilder;
typedef struct _LDClientConfig* LDClientConfig;

LD_EXPORT(LDClientConfigBuilder) LDClientConfigBuilder_New(char const* sdk_key);

LD_EXPORT(LDClientConfig) LDClientConfigBuilder_Build();

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
