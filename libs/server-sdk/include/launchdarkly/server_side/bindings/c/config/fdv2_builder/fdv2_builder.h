/** @file fdv2_builder.h */
// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/export.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if used by C++ source code
#endif

typedef struct _LDServerFDv2Builder* LDServerFDv2Builder;
typedef struct _LDServerFDv2StreamingBuilder* LDServerFDv2StreamingBuilder;
typedef struct _LDServerFDv2PollingBuilder* LDServerFDv2PollingBuilder;

/* Reused as the input handles for LDServerFDv2Builder_FDv1Fallback_*. Fully
 * declared in <launchdarkly/server_side/bindings/c/config/builder.h>. */
typedef struct _LDServerDataSourceStreamBuilder* LDServerDataSourceStreamBuilder;
typedef struct _LDServerDataSourcePollBuilder* LDServerDataSourcePollBuilder;

/**
 * Creates an FDv2 builder pre-populated with the spec-recommended initializers,
 * synchronizers, and FDv1 fallback. Suitable for most applications without
 * further configuration.
 *
 * If not passed into the config builder, must be manually freed with
 * LDServerFDv2Builder_Free.
 *
 * @return A new FDv2 builder with default sources.
 */
LD_EXPORT(LDServerFDv2Builder)
LDServerFDv2Builder_Default(void);

/**
 * Creates an FDv2 builder with no initializers, no synchronizers, and no
 * FDv1 fallback. Sources must be added explicitly via Initializer_*,
 * Synchronizer_*, and FDv1Fallback_* methods.
 *
 * If not passed into the config builder, must be manually freed with
 * LDServerFDv2Builder_Free.
 *
 * @return A new empty FDv2 builder.
 */
LD_EXPORT(LDServerFDv2Builder)
LDServerFDv2Builder_Custom(void);

/**
 * Frees an FDv2 builder. Do not call if the builder was consumed by the
 * config builder.
 *
 * @param b Builder to free.
 */
LD_EXPORT(void)
LDServerFDv2Builder_Free(LDServerFDv2Builder b);

/**
 * Creates a new FDv2 Streaming source builder.
 *
 * If not passed into an FDv2 builder, must be manually freed with
 * LDServerFDv2StreamingBuilder_Free.
 *
 * @return A new FDv2 Streaming source builder.
 */
LD_EXPORT(LDServerFDv2StreamingBuilder)
LDServerFDv2StreamingBuilder_New(void);

/**
 * Sets the initial reconnect delay for the FDv2 streaming connection.
 *
 * The streaming service uses a backoff algorithm (with jitter) every time
 * the connection needs to be reestablished. The delay for the first
 * reconnection will start near this value, and then increase exponentially
 * for any subsequent connection failures.
 *
 * @param b FDv2 Streaming source builder. Must not be NULL.
 * @param milliseconds Initial delay for a reconnection attempt.
 */
LD_EXPORT(void)
LDServerFDv2StreamingBuilder_InitialReconnectDelayMs(
    LDServerFDv2StreamingBuilder b,
    unsigned int milliseconds);

/**
 * Overrides the base URL used by this FDv2 streaming source. By default the
 * streaming source reads its endpoint from the top-level ServiceEndpoints
 * configuration; this override takes precedence for this source only.
 *
 * @param b FDv2 Streaming source builder. Must not be NULL.
 * @param base_url Target URL. Must not be NULL.
 */
LD_EXPORT(void)
LDServerFDv2StreamingBuilder_BaseURL(LDServerFDv2StreamingBuilder b,
                                     char const* base_url);

/**
 * Frees an FDv2 Streaming source builder. Do not call if the builder was
 * consumed by an FDv2 builder.
 *
 * @param b Builder to free.
 */
LD_EXPORT(void)
LDServerFDv2StreamingBuilder_Free(LDServerFDv2StreamingBuilder b);

/**
 * Creates a new FDv2 Polling source builder.
 *
 * If not passed into an FDv2 builder, must be manually freed with
 * LDServerFDv2PollingBuilder_Free.
 *
 * @return A new FDv2 Polling source builder.
 */
LD_EXPORT(LDServerFDv2PollingBuilder)
LDServerFDv2PollingBuilder_New(void);

/**
 * Sets the interval at which this FDv2 polling source will poll for flag
 * updates.
 *
 * @param b FDv2 Polling source builder. Must not be NULL.
 * @param seconds Polling interval in seconds.
 */
LD_EXPORT(void)
LDServerFDv2PollingBuilder_IntervalS(LDServerFDv2PollingBuilder b,
                                         unsigned int seconds);

/**
 * Overrides the base URL used by this FDv2 polling source. By default the
 * polling source reads its endpoint from the top-level ServiceEndpoints
 * configuration; this override takes precedence for this source only.
 *
 * @param b FDv2 Polling source builder. Must not be NULL.
 * @param base_url Target URL. Must not be NULL.
 */
LD_EXPORT(void)
LDServerFDv2PollingBuilder_BaseURL(LDServerFDv2PollingBuilder b,
                                   char const* base_url);

/**
 * Frees an FDv2 Polling source builder. Do not call if the builder was
 * consumed by an FDv2 builder.
 *
 * @param b Builder to free.
 */
LD_EXPORT(void)
LDServerFDv2PollingBuilder_Free(LDServerFDv2PollingBuilder b);

/**
 * Appends an FDv2 polling initializer to the FDv2 builder's initializers
 * list. The source builder is automatically freed.
 *
 * WARNING: Do not call any other LDServerFDv2PollingBuilder function on the
 * provided builder after calling this function. It is undefined behavior.
 *
 * @param b FDv2 builder. Must not be NULL.
 * @param polling The FDv2 Polling source builder. The builder is consumed;
 * do not free it. Must not be NULL.
 */
LD_EXPORT(void)
LDServerFDv2Builder_Initializer_Polling(LDServerFDv2Builder b,
                                        LDServerFDv2PollingBuilder polling);

/**
 * Appends an FDv2 streaming synchronizer to the FDv2 builder's synchronizers
 * list. The source builder is automatically freed.
 *
 * Order in the list determines preference: the first entry is the primary
 * synchronizer, subsequent entries are fallbacks.
 *
 * WARNING: Do not call any other LDServerFDv2StreamingBuilder function on
 * the provided builder after calling this function. It is undefined behavior.
 *
 * @param b FDv2 builder. Must not be NULL.
 * @param streaming The FDv2 Streaming source builder. The builder is
 * consumed; do not free it. Must not be NULL.
 */
LD_EXPORT(void)
LDServerFDv2Builder_Synchronizer_Streaming(
    LDServerFDv2Builder b,
    LDServerFDv2StreamingBuilder streaming);

/**
 * Appends an FDv2 polling synchronizer to the FDv2 builder's synchronizers
 * list. The source builder is automatically freed.
 *
 * See LDServerFDv2Builder_Synchronizer_Streaming for ordering semantics.
 *
 * WARNING: Do not call any other LDServerFDv2PollingBuilder function on the
 * provided builder after calling this function. It is undefined behavior.
 *
 * @param b FDv2 builder. Must not be NULL.
 * @param polling The FDv2 Polling source builder. The builder is consumed;
 * do not free it. Must not be NULL.
 */
LD_EXPORT(void)
LDServerFDv2Builder_Synchronizer_Polling(LDServerFDv2Builder b,
                                         LDServerFDv2PollingBuilder polling);

/**
 * Configures the FDv1 streaming source used as a last-resort fallback when
 * the LaunchDarkly service signals (via the X-LD-FD-Fallback header) that
 * the SDK should switch to FDv1. The fallback reads its endpoint from the
 * top-level ServiceEndpoints configuration. The source builder is
 * automatically freed.
 *
 * The fdv1_streaming parameter uses the same handle type as the
 * BackgroundSync streaming synchronizer
 * (LDServerDataSourceStreamBuilder_New); the underlying C++ type is shared.
 *
 * WARNING: Do not call any other LDServerDataSourceStreamBuilder function
 * on the provided builder after calling this function. It is undefined
 * behavior.
 *
 * @param b FDv2 builder. Must not be NULL.
 * @param fdv1_streaming The FDv1 Streaming source builder. The builder is
 * consumed; do not free it. Must not be NULL.
 */
LD_EXPORT(void)
LDServerFDv2Builder_FDv1Fallback_Streaming(
    LDServerFDv2Builder b,
    LDServerDataSourceStreamBuilder fdv1_streaming);

/**
 * Configures the FDv1 polling source used as a last-resort fallback when
 * the LaunchDarkly service signals (via the X-LD-FD-Fallback header) that
 * the SDK should switch to FDv1. The fallback reads its endpoint from the
 * top-level ServiceEndpoints configuration. The source builder is
 * automatically freed.
 *
 * The fdv1_polling parameter uses the same handle type as the BackgroundSync
 * polling synchronizer (LDServerDataSourcePollBuilder_New); the underlying
 * C++ type is shared.
 *
 * WARNING: Do not call any other LDServerDataSourcePollBuilder function on
 * the provided builder after calling this function. It is undefined behavior.
 *
 * @param b FDv2 builder. Must not be NULL.
 * @param fdv1_polling The FDv1 Polling source builder. The builder is
 * consumed; do not free it. Must not be NULL.
 */
LD_EXPORT(void)
LDServerFDv2Builder_FDv1Fallback_Polling(
    LDServerFDv2Builder b,
    LDServerDataSourcePollBuilder fdv1_polling);

/**
 * Disables the FDv1 fallback. After this call, an FDv1 fallback directive
 * from the service leaves the data source in the interrupted state and
 * schedules an FDv2 retry on the directive's TTL.
 *
 * @param b FDv2 builder. Must not be NULL.
 */
LD_EXPORT(void)
LDServerFDv2Builder_DisableFDv1Fallback(LDServerFDv2Builder b);

/**
 * Sets how long the active synchronizer may remain interrupted before the
 * orchestrator falls back to the next-preferred synchronizer.
 *
 * @param b FDv2 builder. Must not be NULL.
 * @param milliseconds Duration the synchronizer must be continuously
 * interrupted for before fallback fires.
 */
LD_EXPORT(void)
LDServerFDv2Builder_FallbackTimeoutMs(LDServerFDv2Builder b,
                                      unsigned int milliseconds);

/**
 * Sets how long a fallback synchronizer must run successfully before the
 * orchestrator attempts to recover to the primary synchronizer.
 *
 * @param b FDv2 builder. Must not be NULL.
 * @param milliseconds Duration the fallback synchronizer must run before a
 * recovery attempt is made.
 */
LD_EXPORT(void)
LDServerFDv2Builder_RecoveryTimeoutMs(LDServerFDv2Builder b,
                                      unsigned int milliseconds);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
