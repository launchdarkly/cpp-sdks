/** @file fdv2_builder.h */
// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/export.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if used by C++ source code
#endif

typedef struct _LDClientFDv2Builder* LDClientFDv2Builder;
typedef struct _LDClientFDv2ModeBuilder* LDClientFDv2ModeBuilder;
typedef struct _LDClientFDv2StreamingBuilder* LDClientFDv2StreamingBuilder;
typedef struct _LDClientFDv2PollingBuilder* LDClientFDv2PollingBuilder;
typedef struct _LDClientFDv2FDv1FallbackBuilder*
    LDClientFDv2FDv1FallbackBuilder;

/**
 * A named data system configuration: which sources the SDK uses to load flag
 * data, and which it uses to keep that data current.
 */
enum LDClientConnectionMode {
    /** Stream updates, falling back to polling. */
    LD_CLIENT_CONNECTION_MODE_STREAMING = 0,
    /** Poll for updates on an interval. */
    LD_CLIENT_CONNECTION_MODE_POLLING = 1,
    /** Evaluate against whatever is cached, and make no requests. */
    LD_CLIENT_CONNECTION_MODE_OFFLINE = 2
};

/**
 * Creates a new FDv2 builder. It starts from the configuration the SDK uses
 * when the application customizes nothing: streaming mode, with streaming,
 * polling, and offline modes available.
 *
 * If not passed into the config builder, must be manually freed with
 * LDClientFDv2Builder_Free.
 *
 * @return A new FDv2 builder.
 */
LD_EXPORT(LDClientFDv2Builder)
LDClientFDv2Builder_New(void);

/**
 * Frees an FDv2 builder. Do not call if the builder was consumed by the
 * config builder.
 *
 * @param b Builder to free.
 */
LD_EXPORT(void)
LDClientFDv2Builder_Free(LDClientFDv2Builder b);

/**
 * Sets the connection mode the SDK starts in. Defaults to streaming.
 *
 * @param b FDv2 builder. Must not be NULL.
 * @param mode The mode to start in.
 */
LD_EXPORT(void)
LDClientFDv2Builder_InitialMode(LDClientFDv2Builder b,
                                enum LDClientConnectionMode mode);

/**
 * Sends the evaluation context in a request body rather than encoded into the
 * request path. This keeps the context out of URL-based request logs and CDN
 * logs, at the cost of CDN caching.
 *
 * @param b FDv2 builder. Must not be NULL.
 * @param use_post True to send the context in a request body.
 */
LD_EXPORT(void)
LDClientFDv2Builder_UsePost(LDClientFDv2Builder b, bool use_post);

/**
 * Replaces what the given connection mode does. Modes left uncustomized keep
 * their built-in definitions. The mode builder is automatically freed.
 *
 * WARNING: Do not call any other LDClientFDv2ModeBuilder function on the
 * provided builder after calling this function. It is undefined behavior.
 *
 * @param b FDv2 builder. Must not be NULL.
 * @param mode The mode to customize.
 * @param mode_builder The mode definition. The builder is consumed; do not
 * free it. Must not be NULL.
 */
LD_EXPORT(void)
LDClientFDv2Builder_CustomizeMode(LDClientFDv2Builder b,
                                  enum LDClientConnectionMode mode,
                                  LDClientFDv2ModeBuilder mode_builder);

/**
 * Creates a new builder for one connection mode's sources.
 *
 * A mode built this way replaces the SDK's built-in definition entirely, so it
 * should list every source the mode needs, cache included.
 *
 * If not passed into an FDv2 builder, must be manually freed with
 * LDClientFDv2ModeBuilder_Free.
 *
 * @return A new connection mode builder.
 */
LD_EXPORT(LDClientFDv2ModeBuilder)
LDClientFDv2ModeBuilder_New(void);

/**
 * Frees a connection mode builder. Do not call if the builder was consumed by
 * an FDv2 builder.
 *
 * @param b Builder to free.
 */
LD_EXPORT(void)
LDClientFDv2ModeBuilder_Free(LDClientFDv2ModeBuilder b);

/**
 * Appends the local cache to the mode's initializer list. Reading persisted
 * flag data lets evaluation begin before the network answers.
 *
 * @param b Connection mode builder. Must not be NULL.
 */
LD_EXPORT(void)
LDClientFDv2ModeBuilder_Initializer_Cache(LDClientFDv2ModeBuilder b);

/**
 * Appends a polling source to the mode's initializer list. The source builder
 * is automatically freed.
 *
 * Initializers run in order until one loads a complete data set.
 *
 * WARNING: Do not call any other LDClientFDv2PollingBuilder function on the
 * provided builder after calling this function. It is undefined behavior.
 *
 * @param b Connection mode builder. Must not be NULL.
 * @param polling The polling source builder. The builder is consumed; do not
 * free it. Must not be NULL.
 */
LD_EXPORT(void)
LDClientFDv2ModeBuilder_Initializer_Polling(LDClientFDv2ModeBuilder b,
                                            LDClientFDv2PollingBuilder polling);

/**
 * Appends a streaming source to the mode's synchronizer list. The source
 * builder is automatically freed.
 *
 * Order is preference. The first entry is the primary, and the SDK falls back
 * to later entries when it cannot keep the primary running.
 *
 * WARNING: Do not call any other LDClientFDv2StreamingBuilder function on the
 * provided builder after calling this function. It is undefined behavior.
 *
 * @param b Connection mode builder. Must not be NULL.
 * @param streaming The streaming source builder. The builder is consumed; do
 * not free it. Must not be NULL.
 */
LD_EXPORT(void)
LDClientFDv2ModeBuilder_Synchronizer_Streaming(
    LDClientFDv2ModeBuilder b,
    LDClientFDv2StreamingBuilder streaming);

/**
 * Appends a polling source to the mode's synchronizer list. See
 * LDClientFDv2ModeBuilder_Synchronizer_Streaming for ordering semantics. The
 * source builder is automatically freed.
 *
 * WARNING: Do not call any other LDClientFDv2PollingBuilder function on the
 * provided builder after calling this function. It is undefined behavior.
 *
 * @param b Connection mode builder. Must not be NULL.
 * @param polling The polling source builder. The builder is consumed; do not
 * free it. Must not be NULL.
 */
LD_EXPORT(void)
LDClientFDv2ModeBuilder_Synchronizer_Polling(
    LDClientFDv2ModeBuilder b,
    LDClientFDv2PollingBuilder polling);

/**
 * Sets the FDv1 source the mode uses if the service directs the SDK to it. The
 * SDK returns to FDv2 on its own once the service's fallback period has
 * elapsed. The source builder is automatically freed.
 *
 * WARNING: Do not call any other LDClientFDv2FDv1FallbackBuilder function on
 * the provided builder after calling this function. It is undefined behavior.
 *
 * @param b Connection mode builder. Must not be NULL.
 * @param fallback The FDv1 fallback source builder. The builder is consumed;
 * do not free it. Must not be NULL.
 */
LD_EXPORT(void)
LDClientFDv2ModeBuilder_FallbackToFDv1(
    LDClientFDv2ModeBuilder b,
    LDClientFDv2FDv1FallbackBuilder fallback);

/**
 * Leaves the mode with no FDv1 source. A fallback directive then stops the
 * mode's synchronizer until the SDK returns to FDv2.
 *
 * @param b Connection mode builder. Must not be NULL.
 */
LD_EXPORT(void)
LDClientFDv2ModeBuilder_DisableFDv1Fallback(LDClientFDv2ModeBuilder b);

/**
 * Creates a new FDv2 streaming source builder.
 *
 * If not passed into a connection mode builder, must be manually freed with
 * LDClientFDv2StreamingBuilder_Free.
 *
 * @return A new FDv2 streaming source builder.
 */
LD_EXPORT(LDClientFDv2StreamingBuilder)
LDClientFDv2StreamingBuilder_New(void);

/**
 * Sets where the reconnection backoff starts. The delay for the first
 * reconnection starts near this value and grows exponentially for subsequent
 * failures.
 *
 * @param b FDv2 streaming source builder. Must not be NULL.
 * @param milliseconds Initial delay for a reconnection attempt.
 */
LD_EXPORT(void)
LDClientFDv2StreamingBuilder_InitialReconnectDelayMs(
    LDClientFDv2StreamingBuilder b,
    unsigned int milliseconds);

/**
 * Sends this source's requests to the given URL instead of the streaming URL
 * the rest of the SDK uses. Useful for routing one tier to different
 * infrastructure, such as a Relay Proxy used only as a fallback.
 *
 * @param b FDv2 streaming source builder. Must not be NULL.
 * @param base_url Target URL. Must not be NULL.
 */
LD_EXPORT(void)
LDClientFDv2StreamingBuilder_BaseURL(LDClientFDv2StreamingBuilder b,
                                     char const* base_url);

/**
 * Frees an FDv2 streaming source builder. Do not call if the builder was
 * consumed by a connection mode builder.
 *
 * @param b Builder to free.
 */
LD_EXPORT(void)
LDClientFDv2StreamingBuilder_Free(LDClientFDv2StreamingBuilder b);

/**
 * Creates a new FDv2 polling source builder.
 *
 * If not passed into a connection mode builder, must be manually freed with
 * LDClientFDv2PollingBuilder_Free.
 *
 * @return A new FDv2 polling source builder.
 */
LD_EXPORT(LDClientFDv2PollingBuilder)
LDClientFDv2PollingBuilder_New(void);

/**
 * Sets how long to wait between polls. Intervals shorter than the minimum the
 * SDK permits are raised to it.
 *
 * @param b FDv2 polling source builder. Must not be NULL.
 * @param seconds Polling interval in seconds.
 */
LD_EXPORT(void)
LDClientFDv2PollingBuilder_IntervalS(LDClientFDv2PollingBuilder b,
                                     unsigned int seconds);

/**
 * Sends this source's requests to the given URL instead of the polling URL
 * the rest of the SDK uses.
 *
 * @param b FDv2 polling source builder. Must not be NULL.
 * @param base_url Target URL. Must not be NULL.
 */
LD_EXPORT(void)
LDClientFDv2PollingBuilder_BaseURL(LDClientFDv2PollingBuilder b,
                                   char const* base_url);

/**
 * Frees an FDv2 polling source builder. Do not call if the builder was
 * consumed by a connection mode builder.
 *
 * @param b Builder to free.
 */
LD_EXPORT(void)
LDClientFDv2PollingBuilder_Free(LDClientFDv2PollingBuilder b);

/**
 * Creates a new FDv1 fallback source builder.
 *
 * If not passed into a connection mode builder, must be manually freed with
 * LDClientFDv2FDv1FallbackBuilder_Free.
 *
 * @return A new FDv1 fallback source builder.
 */
LD_EXPORT(LDClientFDv2FDv1FallbackBuilder)
LDClientFDv2FDv1FallbackBuilder_New(void);

/**
 * Sets how long to wait between polls while on FDv1.
 *
 * @param b FDv1 fallback source builder. Must not be NULL.
 * @param seconds Polling interval in seconds.
 */
LD_EXPORT(void)
LDClientFDv2FDv1FallbackBuilder_IntervalS(LDClientFDv2FDv1FallbackBuilder b,
                                          unsigned int seconds);

/**
 * Sends the fallback's requests to the given URL instead of the polling URL
 * the rest of the SDK uses.
 *
 * @param b FDv1 fallback source builder. Must not be NULL.
 * @param base_url Target URL. Must not be NULL.
 */
LD_EXPORT(void)
LDClientFDv2FDv1FallbackBuilder_BaseURL(LDClientFDv2FDv1FallbackBuilder b,
                                        char const* base_url);

/**
 * Frees an FDv1 fallback source builder. Do not call if the builder was
 * consumed by a connection mode builder.
 *
 * @param b Builder to free.
 */
LD_EXPORT(void)
LDClientFDv2FDv1FallbackBuilder_Free(LDClientFDv2FDv1FallbackBuilder b);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
