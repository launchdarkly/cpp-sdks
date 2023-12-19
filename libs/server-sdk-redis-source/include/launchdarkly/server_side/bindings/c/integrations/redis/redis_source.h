/** @file redis_source.h
 * @brief LaunchDarkly Server-side Redis Source C Binding.
 */
// NOLINTBEGIN modernize-use-using
#pragma once

#include <launchdarkly/bindings/c/export.h>

#ifdef __cplusplus
extern "C" {
// only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDServerLazyLoadRedisSource* LDServerLazyLoadRedisSource;

/* Defines the size of the error message buffer in LDServerLazyLoadResult.
 */
#ifndef LDSERVER_LAZYLOAD_REDISSOURCE_ERROR_MESSAGE_SIZE
#define LDSERVER_LAZYLOAD_REDISSOURCE_ERROR_MESSAGE_SIZE 256
#endif

/**
 * @brief Stores the result of calling LDDServerLazyLoadRedisSource_New.
 *
 * On successful creation, source will contain a pointer which may be passed
 * into the LaunchDarkly SDK's configuration.
 *
 * On failure, error_message contains a NULL-terminated string describing the
 * error.
 *
 * The message may be truncated if it was originally longer than error_message's
 * buffer size.
 */
struct LDServerLazyLoadRedisResult {
    LDServerLazyLoadRedisSource source;
    char error_message[LDSERVER_LAZYLOAD_REDISSOURCE_ERROR_MESSAGE_SIZE];
};

/**
 * @brief Creates a new Redis data source suitable for usage in the SDK's
 * Lazy Load data system.
 *
 * In this system, the SDK will query Redis for flag/segment
 * data as required, with an in-memory cache to reduce the number of queries.
 *
 * Data is never written back to Redis.
 *
 * @param uri Redis URI string. Must not be NULL or empty string.
 *
 * @param prefix Prefix to use when reading SDK data from Redis. This allows
 * multiple SDK environments to coexist in the same database, or for the SDK's
 * data to coexist with other unrelated data. Must not be NULL.
 *
 * @param out_result Pointer to struct where the source pointer or an error
 * message should be stored.
 *
 * @return True if the source was created successfully; out_result->source
 * will contain the pointer. The caller must either free the
 * pointer with LDServerLazyLoadRedisSource_Free, OR pass it into the SDK's
 * configuration methods which will take ownership (in which case do not call
 * LDServerLazyLoadRedisSource_Free.)
 */
LD_EXPORT(bool)
LDServerLazyLoadRedisSource_New(char const* uri,
                                char const* prefix,
                                struct LDServerLazyLoadRedisResult* out_result);

/**
 * @brief Frees a Redis data source pointer. Only necessary to call if not
 * passing ownership to SDK configuration.
 */
LD_EXPORT(void)
LDServerLazyLoadRedisSource_Free(LDServerLazyLoadRedisSource source);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
