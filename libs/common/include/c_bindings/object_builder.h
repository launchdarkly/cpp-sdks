#pragma once

#include "./export.h"
#include "./value.h"

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef void* LDObjectBuilder;

LD_EXPORT(LDObjectBuilder) LDObjectBuilder_New();
LD_EXPORT(void) LDObjectBuilder_Free(LDObjectBuilder builder);

LD_EXPORT(void)
LDObjectBuilder_Add(LDObjectBuilder builder, char const* key, LDValue val);

LD_EXPORT(LDValue) LDObjectBuilder_Build(LDObjectBuilder builder);

#ifdef __cplusplus
}
#endif
