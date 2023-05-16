// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/config/config.h>
#include <launchdarkly/bindings/c/export.h>
#include <launchdarkly/bindings/c/status.h>

#include <stdio.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDClientConfigBuilder* LDClientConfigBuilder;

typedef struct _LDDataSourceStreamBuilder* LDDataSourceStreamBuilder;

typedef struct _LDDataSourcePollBuilder* LDDataSourcePollBuilder;

/**
 * Constructs a client-side config builder.
 */
LD_EXPORT(LDClientConfigBuilder) LDClientConfigBuilder_New(char const* sdk_key);

/**
 * Sets a custom URL for the polling service.
 * @param b Client config builder. Must not be NULL.
 * @param url Target URL. Must not be NULL.
 */
LD_EXPORT(void)
LDClientConfigBuilder_ServiceEndpoints_PollingBaseURL(LDClientConfigBuilder b,
                                                      char const* url);
/**
 * Sets a custom URL for the streaming service.
 * @param b Client config builder. Must not be NULL.
 * @param url Target URL. Must not be NULL.
 */
LD_EXPORT(void)
LDClientConfigBuilder_ServiceEndpoints_StreamingBaseURL(LDClientConfigBuilder b,
                                                        char const* url);
/**
 * Sets a custom URL for the events service.
 * @param b Client config builder. Must not be NULL.
 * @param url Target URL. Must not be NULL.
 */
LD_EXPORT(void)
LDClientConfigBuilder_ServiceEndpoints_EventsBaseURL(LDClientConfigBuilder b,
                                                     char const* url);
/**
 * Sets a custom URL for a Relay Proxy instance. The streaming,
 * polling, and events URLs are set automatically.
 * @param b Client config builder. Must not be NULL.
 * @param url Target URL. Must not be NULL.
 */
LD_EXPORT(void)
LDClientConfigBuilder_ServiceEndpoints_RelayProxy(LDClientConfigBuilder b,
                                                  char const* url);

/**
 * Sets an identifier for the application.
 * @param b Client config builder. Must not be NULL.
 * @param app_id Non-empty string. Must be <= 64 chars. Must be alphanumeric,
 * '-', '.', or '_'. Must not be NULL.
 */
LD_EXPORT(void)
LDClientConfigBuilder_AppInfo_Identifier(LDClientConfigBuilder b,
                                         char const* app_id);

/**
 * Sets a version for the application.
 * @param b Client config builder. Must not be NULL.
 * @param app_version Non-empty string. Must be <= 64 chars. Must be
 * alphanumeric,
 * '-', '.', or '_'. Must not be NULL.
 */
LD_EXPORT(void)
LDClientConfigBuilder_AppInfo_Version(LDClientConfigBuilder b,
                                      char const* app_version);

/**
 * Enables or disables "Offline" mode. True means
 * Offline mode is enabled.
 * @param b Client config builder. Must not be NULL.
 * @param offline True if offline.
 */
LD_EXPORT(void)
LDClientConfigBuilder_Offline(LDClientConfigBuilder b, bool offline);

/**
 * Specify if event-sending should be enabled or not. By default,
 * events are enabled.
 * @param b Client config builder. Must not be NULL.
 * @param enabled True to enable event-sending.
 */
LD_EXPORT(void)
LDClientConfigBuilder_Events_Enabled(LDClientConfigBuilder b, bool enabled);

/**
 * Sets the capacity of the event processor. When more events are generated
 * within the processor's flush interval than this value, events will be
 * dropped.
 * @param b Client config builder. Must not be NULL.
 * @param capacity Event queue capacity.
 */
LD_EXPORT(void)
LDClientConfigBuilder_Events_Capacity(LDClientConfigBuilder b, size_t capacity);

/**
 * Sets the flush interval of the event processor. The processor queues
 * outgoing events based on the capacity parameter; these events are then
 * delivered based on the flush interval.
 * @param b Client config builder. Must not be NULL.
 * @param milliseconds Interval between automatic flushes.
 */
LD_EXPORT(void)
LDClientConfigBuilder_Events_FlushIntervalMs(LDClientConfigBuilder b,
                                             unsigned int milliseconds);

/**
 * Attribute privacy indicates whether or not attributes should be
 * retained by LaunchDarkly after being sent upon initialization,
 * and if attributes should later be sent in events.
 *
 * Attribute privacy may be specified in 3 ways:
 *
 * (1) To specify that all attributes should be considered private - not
 * just those designated private on a per-context basis - call this method
 * with true as the parameter.
 *
 * (2) To specify that a specific set of attributes should be considered
 * private - in addition to those designated private on a per-context basis
 * - call @ref PrivateAttribute.
 *
 * (3) To specify private attributes on a per-context basis, it is not
 * necessary to call either of these methods, as the default behavior is to
 * treat all attributes as non-private unless otherwise specified.
 *
 * @param b Client config builder. Must not be NULL.
 * @param all_attributes_private True for behavior of (1), false for default
 * behavior of (2) or (3).
 */
LD_EXPORT(void)
LDClientConfigBuilder_Events_AllAttributesPrivate(LDClientConfigBuilder b,
                                                  bool all_attributes_private);

/**
 * Specifies a single private attribute. May be called multiple times
 * with additional private attributes.
 * @param b Client config builder. Must not be NULL.
 * @param attribute_reference Attribute to mark private.
 */
LD_EXPORT(void)
LDClientConfigBuilder_Events_PrivateAttribute(LDClientConfigBuilder b,
                                              char const* attribute_reference);
/**
 * * Whether LaunchDarkly should provide additional information about how flag
 * values were calculated.
 *
 * The additional information will then be available through the client's
 * VariationDetail methods. Since this increases the size of network
 * requests, such information is not sent unless you set this option to
 * true.
 * @param b Client config builder. Must not be NULL.
 * @param with_reasons True to enable reasons.
 */
LD_EXPORT(void)
LDClientConfigBuilder_DataSource_WithReasons(LDClientConfigBuilder b,
                                             bool with_reasons);

/**
 * Whether or not to use the REPORT verb to fetch flag settings.
 *
 * If this is true, flag settings will be fetched with a REPORT request
 * including a JSON entity body with the context object.
 *
 * Otherwise (by default) a GET request will be issued with the context
 * passed as a base64 URL-encoded path parameter.
 *
 * Do not use unless advised by LaunchDarkly.
 * @param b Client config builder. Must not be NULL.
 * @param use_reasons True to use the REPORT verb.
 */
LD_EXPORT(void)
LDClientConfigBuilder_DataSource_UseReport(LDClientConfigBuilder b,
                                           bool use_report);
/**
 * Set the streaming configuration for the builder.
 *
 * A data source may either be streaming or polling. Setting a streaming
 * builder indicates the data source will use streaming. Setting a polling
 * builder will indicate the use of polling.
 *
 * @param b Client config builder. Must not be NULL.
 * @param stream_builder The streaming builder. The builder is consumed; do not
 * free it.
 */
LD_EXPORT(void)
LDClientConfigBuilder_DataSource_MethodStream(
    LDClientConfigBuilder b,
    LDDataSourceStreamBuilder stream_builder);

/**
 * Set the polling configuration for the builder.
 *
 * A data source may either be streaming or polling. Setting a stream
 * builder indicates the data source will use streaming. Setting a polling
 * builder will indicate the use of polling.
 *
 * @param b  Client config builder. Must not be NULL.
 * @param poll_builder The polling builder. The builder is consumed; do not free
 * it.
 */
LD_EXPORT(void)
LDClientConfigBuilder_DataSource_MethodPoll(
    LDClientConfigBuilder b,
    LDDataSourcePollBuilder poll_builder);

/**
 * Creates a new DataSource builder for the Streaming method.
 *
 * If not passed into the config
 * builder, must be manually freed with LDDataSourceStreamBuilder_Free.
 *
 * @return New builder for Streaming method.
 */
LD_EXPORT(LDDataSourceStreamBuilder)
LDDataSourceStreamBuilder_New();

/**
 * Sets the initial reconnect delay for the streaming connection.
 *
 * The streaming service uses a backoff algorithm (with jitter) every time
 * the connection needs to be reestablished.The delay for the first
 * reconnection will start near this value, and then increase exponentially
 * for any subsequent connection failures.
 *
 * @param b Streaming method builder.
 * @param milliseconds Initial delay for a reconnection attempt.
 */
LD_EXPORT(void)
LDDataSourceStreamBuilder_InitialReconnectDelayMs(LDDataSourceStreamBuilder b,
                                                  unsigned int milliseconds);

/**
 * Frees a Streaming method builder. Do not call if the builder was consumed by
 * the config builder.
 *
 * @param b Builder to free.
 */
LD_EXPORT(void) LDDataSourceStreamBuilder_Free(LDDataSourceStreamBuilder b);

/**
 * Creates a new DataSource builder for the Polling method.
 *
 * If not passed into the config
 * builder, must be manually freed with LDDataSourcePollBuilder_Free.
 *
 * @return New builder for Polling method.
 */

LD_EXPORT(LDDataSourcePollBuilder)
LDDataSourcePollBuilder_New();

/**
 * Sets the interval at which the SDK will poll for feature flag updates.
 * @param b Polling method builder.
 * @param milliseconds Polling interval.
 */
LD_EXPORT(void)
LDDataSourcePollBuilder_IntervalS(LDDataSourcePollBuilder b,
                                  unsigned int seconds);

/**
 * Frees a Polling method builder. Do not call if the builder was consumed by
 * the config builder.
 *
 * @param b Builder to free.
 */
LD_EXPORT(void) LDDataSourcePollBuilder_Free(LDDataSourcePollBuilder b);

/**
 * Creates an LDClientConfig. The LDClientConfigBuilder is consumed.
 * On success, the config will be stored in out_config; otherwise,
 * out_config will be set to NULL and the returned LDStatus will indicate
 * the error.
 * @param builder Builder to consume. Must not be NULL.
 * @param out_config Pointer to where the built config will be
 * stored. Must not be NULL.
 * @return Error status on failure.
 */
LD_EXPORT(LDStatus)
LDClientConfigBuilder_Build(LDClientConfigBuilder builder,
                            LDClientConfig* out_config);

/**
 * Frees the builder; only necessary if not calling Build.
 * @param builder Builder to free.
 */
LD_EXPORT(void)
LDClientConfigBuilder_Free(LDClientConfigBuilder builder);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
