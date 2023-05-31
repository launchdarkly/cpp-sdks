/** @file */
// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/export.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

LD_EXPORT(void)
LDMemory_FreeString(char* string);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
