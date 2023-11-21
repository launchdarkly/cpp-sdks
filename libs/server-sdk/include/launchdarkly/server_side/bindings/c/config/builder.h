/** @file */
// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/server_side/bindings/c/config/config.h>

#include <launchdarkly/bindings/c/config/logging_builder.h>
#include <launchdarkly/bindings/c/export.h>
#include <launchdarkly/bindings/c/status.h>

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDServerConfigBuilder* LDServerConfigBuilder;
typedef struct _LDServerStreamingSyncBuilder* LDServerStreamingSyncBuilder;
typedef struct _LDServerPollingSyncBuilder* LDServerPollingSyncBuilder;

/**
 * Constructs a client-side config builder.
 */
LD_EXPORT(LDServerConfigBuilder) LDServerConfigBuilder_New(char const* sdk_key);

/**
 * Sets a custom URL for the polling service.
 * @param b Config builder. Must not be NULL.
 * @param url Target URL. Must not be NULL.
 */
LD_EXPORT(void)
LDServerConfigBuilder_ServiceEndpoints_PollingBaseURL(LDServerConfigBuilder b,
                                                      char const* url);
/**
 * Sets a custom URL for the streaming service.
 * @param b Config builder. Must not be NULL.
 * @param url Target URL. Must not be NULL.
 */
LD_EXPORT(void)
LDServerConfigBuilder_ServiceEndpoints_StreamingBaseURL(LDServerConfigBuilder b,
                                                        char const* url);
/**
 * Sets a custom URL for the events service.
 * @param b Config builder. Must not be NULL.
 * @param url Target URL. Must not be NULL.
 */
LD_EXPORT(void)
LDServerConfigBuilder_ServiceEndpoints_EventsBaseURL(LDServerConfigBuilder b,
                                                     char const* url);
/**
 * Sets a custom URL for a Relay Proxy instance. The streaming,
 * polling, and events URLs are set automatically.
 * @param b Config builder. Must not be NULL.
 * @param url Target URL. Must not be NULL.
 */
LD_EXPORT(void)
LDServerConfigBuilder_ServiceEndpoints_RelayProxyBaseURL(
    LDServerConfigBuilder b,
    char const* url);

/**
 * Sets an identifier for the application.
 * @param b Config builder. Must not be NULL.
 * @param app_id Non-empty string. Must be <= 64 chars. Must be alphanumeric,
 * '-', '.', or '_'. Must not be NULL.
 */
LD_EXPORT(void)
LDServerConfigBuilder_AppInfo_Identifier(LDServerConfigBuilder b,
                                         char const* app_id);

/**
 * Sets a version for the application.
 * @param b Config builder. Must not be NULL.
 * @param app_version Non-empty string. Must be <= 64 chars. Must be
 * alphanumeric,
 * '-', '.', or '_'. Must not be NULL.
 */
LD_EXPORT(void)
LDServerConfigBuilder_AppInfo_Version(LDServerConfigBuilder b,
                                      char const* app_version);

/**
 * If true, equivalent to setting LDServerConfigBuilder_Events_Enabled(false)
 * and LDServerConfigBuilder_DataSystem_Enabled(false).
 *
 * The effect is that all evaluations will return
 * application-provided default values, and no network calls will be made.
 *
 * This overrides specific configuration of events and/or data system, if
 * present.
 * @param b Config builder. Must not be NULL.
 * @param offline True if offline.
 */
LD_EXPORT(void)
LDServerConfigBuilder_Offline(LDServerConfigBuilder b, bool offline);

/**
 * Specify if event-sending should be enabled or not. By default,
 * events are enabled.
 * @param b Config builder. Must not be NULL.
 * @param enabled True to enable event-sending.
 */
LD_EXPORT(void)
LDServerConfigBuilder_Events_Enabled(LDServerConfigBuilder b, bool enabled);

/**
 * Sets the capacity of the event processor. When more events are generated
 * within the processor's flush interval than this value, events will be
 * dropped.
 * @param b Config builder. Must not be NULL.
 * @param capacity Event queue capacity.
 */
LD_EXPORT(void)
LDServerConfigBuilder_Events_Capacity(LDServerConfigBuilder b, size_t capacity);

/**
 * Sets the flush interval of the event processor. The processor queues
 * outgoing events based on the capacity parameter; these events are then
 * delivered based on the flush interval.
 * @param b Config builder. Must not be NULL.
 * @param milliseconds Interval between automatic flushes.
 */
LD_EXPORT(void)
LDServerConfigBuilder_Events_FlushIntervalMs(LDServerConfigBuilder b,
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
 * @param b Config builder. Must not be NULL.
 * @param all_attributes_private True for behavior of (1), false for default
 * behavior of (2) or (3).
 */
LD_EXPORT(void)
LDServerConfigBuilder_Events_AllAttributesPrivate(LDServerConfigBuilder b,
                                                  bool all_attributes_private);

/**
 * Specifies a single private attribute. May be called multiple times
 * with additional private attributes.
 * @param b Config builder. Must not be NULL.
 * @param attribute_reference Attribute to mark private.
 */
LD_EXPORT(void)
LDServerConfigBuilder_Events_PrivateAttribute(LDServerConfigBuilder b,
                                              char const* attribute_reference);

/**
 * Configures the Background Sync data system with a Streaming synchronizer.
 *
 * This is the default data system for the SDK.
 *
 * In this mode, the SDK maintains a persistent, streaming data connection
 * with LaunchDarkly. Flag data is received automatically in the background,
 * meaning there are no network costs associated with evaluating flags.
 *
 * @param b Config builder. Must not be NULL.
 * @param stream_builder The streaming builder. The builder is consumed; do not
 * free it.
 */
LD_EXPORT(void)
LDServerConfigBuilder_DataSystem_BackgroundSync_Streaming(
    LDServerConfigBuilder b,
    LDServerStreamingSyncBuilder stream_builder);

/**
 * Configures the Background Sync data system with a Polling synchronizer.
 *
 * This synchronizer may be chosen to override the default Streaming mode.
 *
 * In this mode, the SDK makes periodic network requests to LaunchDarkly.
 * Between requests, flag data may be stale to some degree. This mode may be
 * advantageous if a streaming connection cannot be maintained.
 *
 * @param b Config builder. Must not be NULL.
 * @param poll_builder The polling builder. The builder is consumed; do not free
 * it.
 */
LD_EXPORT(void)
LDServerConfigBuilder_DataSystem_BackgroundSync_Polling(
    LDServerConfigBuilder b,
    LDServerPollingSyncBuilder poll_builder);

/**
 * Specify if the SDK's data system should be enabled or not.
 *
 * By default, the SDK uses the Background Sync data system with a Streaming
 * connection.
 *
 * @param b Config builder. Must not be NULL.
 * @param enabled True to enable the data system, false to disable it.
 */
LD_EXPORT(void)
LDServerConfigBuilder_DataSystem_Enabled(LDServerConfigBuilder b, bool enabled);

/**
 * Creates a new DataSource builder for the Streaming method.
 *
 * If not passed into the config
 * builder, must be manually freed with LDServerStreamingSyncBuilder_Free.
 *
 * @return New builder for Streaming method.
 */
LD_EXPORT(LDServerStreamingSyncBuilder)
LDServerStreamingSyncBuilder_New();

/**
 * Sets the initial reconnect delay for the streaming connection.
 *
 * The streaming service uses a backoff algorithm (with jitter) every time
 * the connection needs to be reestablished.The delay for the first
 * reconnection will start near this value, and then increase exponentially
 * for any subsequent connection failures.
 *
 * @param b Streaming method builder. Must not be NULL.
 * @param milliseconds Initial delay for a reconnection attempt.
 */
LD_EXPORT(void)
LDServerStreamingSyncBuilder_InitialReconnectDelayMs(
    LDServerStreamingSyncBuilder b,
    unsigned int milliseconds);

/**
 * Frees a Streaming method builder. Do not call if the builder was consumed by
 * the config builder.
 *
 * @param b Builder to free.
 */
LD_EXPORT(void)
LDServerStreamingSyncBuilder_Free(LDServerStreamingSyncBuilder b);

/**
 * Creates a new DataSource builder for the Polling method.
 *
 * If not passed into the config
 * builder, must be manually freed with LDServerPollingSyncBuilder_Free.
 *
 * @return New builder for Polling method.
 */

LD_EXPORT(LDServerPollingSyncBuilder)
LDServerPollingSyncBuilder_New();

/**
 * Sets the interval at which the SDK will poll for feature flag updates.
 * @param b Polling method builder. Must not be NULL.
 * @param milliseconds Polling interval.
 */
LD_EXPORT(void)
LDServerPollingSyncBuilder_IntervalS(LDServerPollingSyncBuilder b,
                                     unsigned int seconds);

/**
 * Frees a Polling method builder. Do not call if the builder was consumed by
 * the config builder.
 *
 * @param b Builder to free.
 */
LD_EXPORT(void)
LDServerPollingSyncBuilder_Free(LDServerPollingSyncBuilder b);

/**
 * This should be used for wrapper SDKs to set the wrapper name.
 *
 * Wrapper information will be included in request headers.
 * @param b Config builder. Must not be NULL.
 * @param wrapper_name Name of the wrapper.
 */
LD_EXPORT(void)
LDServerConfigBuilder_HttpProperties_WrapperName(LDServerConfigBuilder b,
                                                 char const* wrapper_name);

/**
 * This should be used for wrapper SDKs to set the wrapper version.
 *
 * Wrapper information will be included in request headers.
 * @param b Config builder. Must not be NULL.
 * @param wrapper_version Version of the wrapper.
 */
LD_EXPORT(void)
LDServerConfigBuilder_HttpProperties_WrapperVersion(
    LDServerConfigBuilder b,
    char const* wrapper_version);

/**
 * Set a custom header value. May be called more than once with additional
 * headers.
 *
 * @param b Config builder. Must not be NULL.
 * @param key Name of the header. Must not be NULL.
 * @param value Value of the header. Must not be NULL.
 */
LD_EXPORT(void)
LDServerConfigBuilder_HttpProperties_Header(LDServerConfigBuilder b,
                                            char const* key,
                                            char const* value);

/**
 * Disables the default SDK logging.
 * @param b Config builder. Must not be NULL.
 */
LD_EXPORT(void)
LDServerConfigBuilder_Logging_Disable(LDServerConfigBuilder b);

/**
 * Configures the SDK with basic logging.
 * @param b  Config builder. Must not be NULL.
 * @param basic_builder The basic logging builder. Must not be NULL.
 */
LD_EXPORT(void)
LDServerConfigBuilder_Logging_Basic(LDServerConfigBuilder b,
                                    LDLoggingBasicBuilder basic_builder);

/**
 * Configures the SDK with custom logging.
 * @param b  Config builder. Must not be NULL.
 * @param custom_builder The custom logging builder. Must not be NULL.
 */
LD_EXPORT(void)
LDServerConfigBuilder_Logging_Custom(LDServerConfigBuilder b,
                                     LDLoggingCustomBuilder custom_builder);

/**
 * Creates an LDClientConfig. The LDServerConfigBuilder is consumed.
 * On success, the config will be stored in out_config; otherwise,
 * out_config will be set to NULL and the returned LDStatus will indicate
 * the error.
 * @param builder Builder to consume. Must not be NULL.
 * @param out_config Pointer to where the built config will be
 * stored. Must not be NULL.
 * @return Error status on failure.
 */
LD_EXPORT(LDStatus)
LDServerConfigBuilder_Build(LDServerConfigBuilder builder,
                            LDServerConfig* out_config);

/**
 * Frees the builder; only necessary if not calling Build.
 * @param builder Builder to free.
 */
LD_EXPORT(void)
LDServerConfigBuilder_Free(LDServerConfigBuilder builder);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
