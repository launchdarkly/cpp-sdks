/** @file redis_source.h
* @brief LaunchDarkly Server-side Redis Source C Binding.
 */
// NOLINTBEGIN modernize-use-using
#pragma once

#include <launchdarkly/bindings/c/export.h>
#include <launchdarkly/bindings/c/memory_routines.h>
#include <launchdarkly/bindings/c/status.h>

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
// only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDServerSDK_RedisSource* LDServerSDK_RedisSource;

LD_EXPORT(LDStatus) LDServerSDK_RedisSource_Create(
    const char* uri,
    const char* prefix,
    LDServerSDK_RedisSource* out_source,
    char** out_exception_msg);


#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
