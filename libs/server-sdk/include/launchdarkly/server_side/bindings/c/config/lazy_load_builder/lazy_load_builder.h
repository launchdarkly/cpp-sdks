/** @file lazy_load_builder.h */
// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/export.h>

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDServerLazyLoadBuilder* LDServerLazyLoadBuilder;

typedef struct _LDServerLazyLoadSourcePtr* LDServerLazyLoadSourcePtr;

/**
 * @brief Specifies the action taken when a data item within the
 * in-memory cache expires.
 *
 * At this time, the default policy is the only supported policy so there
 * is no need to explicitely set it.
 */
enum LDLazyLoadCacheEvictionPolicy {
    /* No action taken; cache eviction is disabled. Stale items will be used
     * in evaluations if they cannot be refreshed. */
    LD_LAZYLOAD_CACHE_EVICTION_POLICY_DISABLED = 0
};

/**
 * Creates a Lazy Load builder which can be used as the SDK's data system.
 *
 * In Lazy Load mode, the SDK will query a source for data as required, with an
 * in-memory cache to reduce the number of queries. This enables usage of
 * databases for storing feature flag/segment data.
 *
 * In contrast, the Background Sync system injects data into the SDK
 * asynchronously (either instantly as updates happen, in streaming mode; or
 * periodically, in polling mode).
 *
 * Background Sync mode is preferred for most use cases, but Lazy Load may be
 * beneficial when no connection to LaunchDarkly is required, such as when using
 * the Relay Proxy to populate a database.
 */
LD_EXPORT(LDServerLazyLoadBuilder)
LDServerLazyLoadBuilder_New();

/**
 * @brief Frees the memory associated with a Lazy Load builder. Do not call if
 * the builder was consumed by the SDK config builder.
 * @param b The builder to free.
 */
LD_EXPORT(void)
LDServerLazyLoadBuilder_Free(LDServerLazyLoadBuilder b);

/**
 * Configures the Lazy Load system with a source via opaque pointer to
 * C++ ISerializedDataReader.
 *
 * @param b The builder. Must not be NULL.
 * @param source The source pointer. Behavior is undefined if the pointer is not
 * an ISerializedDataReader. Must not be NULL.
 */
LD_EXPORT(void)
LDServerLazyLoadBuilder_SourcePtr(LDServerLazyLoadBuilder b,
                                  LDServerLazyLoadSourcePtr source);

/**
 * @brief Specify the duration data items should live in-memory
 * before requiring a refresh via the database. The chosen @ref
 * LDLazyLoadCacheEvictionPolicy affects usage of this TTL.
 * @param b The builder. Must not be NULL.
 * @param milliseconds The time-to-live for an item in milliseconds.
 */
LD_EXPORT(void)
LDServerLazyLoadBuilder_CacheRefreshMs(LDServerLazyLoadBuilder b,
                                       unsigned int milliseconds);

/**
 * @brief Specify the eviction policy when a data item's TTL expires.
 * At this time, only LD_LAZYLOAD_CACHE_EVICTION_POLICY_DISABLED is supported
 * (the default), which leaves stale items in the cache until they can be
 * refreshed.
 * @param b The builder. Must not be NULL.
 * @param policy The eviction policy.
 */
LD_EXPORT(void)
LDServerLazyLoadBuilder_CachePolicy(LDServerLazyLoadBuilder b,
                                    enum LDLazyLoadCacheEvictionPolicy policy);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
