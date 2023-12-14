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

typedef struct _LDServerLazyLoadRedisSource* LDServerLazyLoadRedisSource;


struct LDServerLazyLoadResult {
    LDServerLazyLoadRedisSource source;
    char exception_msg[256];
};

/**
 * @brief Creates a new Redis data source suitable for usage in the SDK's
 * Lazy Load data system. At runtime, the SDK will query Redis for flag/segment
 * data as required, with an in-memory cache to reduce the number of queries. Data
 * is never written back to Redis.
 * @param uri Redis URI string. Must not be NULL or empty string.
 * @param prefix Prefix to use when reading SDK data from Redis. This allows multiple
 * SDK environments to coexist in the same database, or for the SDK's data to coexist with
 * other unrelated data. Must not be NULL.
 * @param out_source Pointer to location where the Redis source pointer should be
 * stored. Must not be NULL.
 * @param out_exception_msg Pointer to location where an exception message should be stored
 * if the source cannot be created.
 * @return LDStatus indicating the result of the operation. If the operation fails, then out_source
 * and out_exception_msg will be set and the status must be freed using
 * LDStatus_Free.
 */
LD_EXPORT(bool) LDServerLazyLoadRedisSource_New(
    const char* uri,
    const char* prefix,
    LDServerLazyLoadResult* out_result);

LD_EXPORT(void) LDServerLazyLoadRedisSource_Free(
    LDServerLazyLoadRedisSource source);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
