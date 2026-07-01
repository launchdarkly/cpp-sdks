/** @file redis_big_segment_store.h
 * @brief LaunchDarkly Server-side Redis Big Segments Store C Binding.
 */
// NOLINTBEGIN modernize-use-using
#pragma once

#include <launchdarkly/bindings/c/export.h>

#ifdef __cplusplus
extern "C" {
// only need to export C interface if
// used by C++ source code
#endif

/**
 * @brief LDServerBigSegmentsRedisStore is a Big Segments persistent store for
 * the Server-Side SDK backed by Redis. It is meant to be passed to the SDK
 * via the Big Segments config builder.
 *
 * Call @ref LDServerBigSegmentsRedisStore_New to obtain a new instance. This
 * instance is passed into the SDK's Big Segments configuration.
 *
 * Example:
 * @code
 *  // Stack allocate the result struct, which will hold the result pointer or
 *  // an error message.
 *  struct LDServerBigSegmentsRedisResult result;
 *
 *  // Create the Redis Big Segment store, passing in arguments for the URI,
 *  // prefix, and pointer to the result.
 *  if (!LDServerBigSegmentsRedisStore_New("redis://localhost:6379",
 *                                         "testprefix", &result)) {
 *      // On failure, you may print the error message (result.error_message),
 *      // then exit or return.
 *  }
 *
 *  // Create the Big Segments builder, taking ownership of the store pointer.
 *  LDServerBigSegmentsBuilder bs_builder = LDServerBigSegmentsBuilder_New(
 *      (LDServerBigSegmentStorePtr)result.store);
 *
 *  // Attach the Big Segments builder to the SDK config.
 *  LDServerConfigBuilder cfg_builder = LDServerConfigBuilder_New("sdk-123");
 *  LDServerConfigBuilder_BigSegments(cfg_builder, bs_builder);
 * @endcode
 *
 * This implementation is backed by <a
 * href="https://github.com/sewenew/redis-plus-plus">Redis++</a>, a C++ wrapper
 * for the <a href="https://github.com/redis/hiredis">hiredis</a> library.
 */
typedef struct _LDServerBigSegmentsRedisStore* LDServerBigSegmentsRedisStore;

/* Defines the size of the error message buffer in
 * LDServerBigSegmentsRedisResult.
 */
#ifndef LDSERVER_BIGSEGMENTS_REDISSTORE_ERROR_MESSAGE_SIZE
#define LDSERVER_BIGSEGMENTS_REDISSTORE_ERROR_MESSAGE_SIZE 256
#endif

/**
 * @brief Stores the result of calling @ref LDServerBigSegmentsRedisStore_New.
 *
 * On successful creation, store will contain a pointer which may be passed
 * into the LaunchDarkly SDK's Big Segments configuration.
 *
 * On failure, error_message contains a NULL-terminated string describing the
 * error.
 *
 * The message may be truncated if it was originally longer than
 * error_message's buffer size.
 */
struct LDServerBigSegmentsRedisResult {
    LDServerBigSegmentsRedisStore store;
    char error_message[LDSERVER_BIGSEGMENTS_REDISSTORE_ERROR_MESSAGE_SIZE];
};

/**
 * @brief Creates a new Redis Big Segment store suitable for usage in the SDK's
 * Big Segments configuration.
 *
 * In this system, the SDK will query Redis for Big Segments membership
 * as required, with an in-memory cache to reduce the number of queries.
 *
 * Data is never written back to Redis by the SDK; the LaunchDarkly Relay
 * Proxy populates the store.
 *
 * @param uri Redis URI string. Must not be NULL or empty string.
 *
 * @param prefix Prefix to use when reading SDK data from Redis. This allows
 * multiple SDK environments to coexist in the same database, or for the SDK's
 * data to coexist with other unrelated data. Must not be NULL.
 *
 * @param out_result Pointer to struct where the store pointer or an error
 * message should be stored.
 *
 * @return True if the store was created successfully; out_result->store
 * will contain the pointer. The caller must either free the pointer with
 * @ref LDServerBigSegmentsRedisStore_Free, OR pass it into the SDK's Big
 * Segments configuration which will take ownership (in which case do not
 * call @ref LDServerBigSegmentsRedisStore_Free.)
 */
LD_EXPORT(bool)
LDServerBigSegmentsRedisStore_New(
    char const* uri,
    char const* prefix,
    struct LDServerBigSegmentsRedisResult* out_result);

/**
 * @brief Frees a Redis Big Segment store pointer. Only necessary to call if
 * not passing ownership to the SDK's Big Segments configuration.
 */
LD_EXPORT(void)
LDServerBigSegmentsRedisStore_Free(LDServerBigSegmentsRedisStore store);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
