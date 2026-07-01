/** @file big_segments_builder.h */
// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/export.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if used by C++ source code
#endif

typedef struct _LDServerBigSegmentsBuilder* LDServerBigSegmentsBuilder;

/**
 * Opaque handle to a Big Segment store implementation, produced by an
 * integration library (for example, the server-side Redis or DynamoDB Big
 * Segments source libraries).
 *
 * Behavior is undefined if the pointer does not originate from a
 * LaunchDarkly-provided Big Segments integration.
 */
typedef struct _LDServerBigSegmentStorePtr* LDServerBigSegmentStorePtr;

/**
 * Creates a Big Segments builder wrapping the given store.
 *
 * The store pointer is consumed: the SDK takes ownership of the underlying
 * store and will free it when the SDK is destroyed. Do not free the store
 * pointer after calling this function.
 *
 * If not passed into the config builder, the returned Big Segments builder
 * must be manually freed with LDServerBigSegmentsBuilder_Free.
 *
 * @param store Big Segment store handle. Must not be NULL.
 * @return A new Big Segments builder.
 */
LD_EXPORT(LDServerBigSegmentsBuilder)
LDServerBigSegmentsBuilder_New(LDServerBigSegmentStorePtr store);

/**
 * Sets the maximum number of context membership lookups cached by the SDK.
 * Defaults to 1000.
 *
 * A higher value reduces store queries for recently-referenced contexts at
 * the cost of memory.
 *
 * @param b Big Segments builder. Must not be NULL.
 * @param size Maximum cached lookups.
 */
LD_EXPORT(void)
LDServerBigSegmentsBuilder_ContextCacheSize(LDServerBigSegmentsBuilder b,
                                            size_t size);

/**
 * Sets the time-to-live for cached membership lookups. Defaults to 5 seconds.
 *
 * A higher value reduces store queries for any given context, but delays
 * the SDK noticing membership changes. Zero is coerced to the default.
 *
 * @param b Big Segments builder. Must not be NULL.
 * @param milliseconds Cache time-to-live.
 */
LD_EXPORT(void)
LDServerBigSegmentsBuilder_ContextCacheTimeMs(LDServerBigSegmentsBuilder b,
                                              unsigned int milliseconds);

/**
 * Sets the interval at which the SDK polls the store's metadata to determine
 * availability and staleness. Defaults to 5 seconds.
 *
 * Zero is coerced to the default.
 *
 * @param b Big Segments builder. Must not be NULL.
 * @param milliseconds Poll interval.
 */
LD_EXPORT(void)
LDServerBigSegmentsBuilder_StatusPollIntervalMs(LDServerBigSegmentsBuilder b,
                                                unsigned int milliseconds);

/**
 * Sets how long the SDK waits before treating store data as stale.
 * Defaults to 2 minutes.
 *
 * If the store's last-updated timestamp falls behind the current time by
 * more than this duration, evaluations report a big segments status of
 * STALE and the status provider reports the store as stale. Zero is coerced
 * to the default.
 *
 * @param b Big Segments builder. Must not be NULL.
 * @param milliseconds Staleness threshold.
 */
LD_EXPORT(void)
LDServerBigSegmentsBuilder_StaleAfterMs(LDServerBigSegmentsBuilder b,
                                        unsigned int milliseconds);

/**
 * Frees a Big Segments builder. Do not call if the builder was consumed by
 * the config builder.
 *
 * @param b Builder to free.
 */
LD_EXPORT(void)
LDServerBigSegmentsBuilder_Free(LDServerBigSegmentsBuilder b);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
