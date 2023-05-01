#pragma once

#include "./export.h"
#include "./value.h"

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef void* LDArrayBuilder;

LD_EXPORT(LDArrayBuilder) LDArrayBuilder_New();
LD_EXPORT(void) LDArrayBuilder_Free(LDArrayBuilder array_builder);

LD_EXPORT(void) LDArrayBuilder_Add(LDArrayBuilder array_builder, LDValue val);

LD_EXPORT(LDValue) LDArrayBuilder_Build(LDArrayBuilder array_builder);

#ifdef __cplusplus
}
#endif
