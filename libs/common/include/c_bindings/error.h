// NOLINTBEGIN modernize-use-using

#pragma once

#include "./export.h"

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDError* LDError;

LD_EXPORT(char const*) LDError_ToString(LDError error);

LD_EXPORT(void) LDError_Free(LDError error);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
