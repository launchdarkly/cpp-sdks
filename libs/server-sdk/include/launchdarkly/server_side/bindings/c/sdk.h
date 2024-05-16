/** @file sdk.h
 * @brief LaunchDarkly Server-side C Bindings.
 */
// NOLINTBEGIN modernize-use-using
#pragma once

#include <launchdarkly/server_side/bindings/c/all_flags_state/all_flags_state.h>
#include <launchdarkly/server_side/bindings/c/config/config.h>

#include <launchdarkly/bindings/c/context.h>
#include <launchdarkly/bindings/c/data/evaluation_detail.h>
#include <launchdarkly/bindings/c/data_source/error_info.h>
#include <launchdarkly/bindings/c/export.h>
#include <launchdarkly/bindings/c/listener_connection.h>
#include <launchdarkly/bindings/c/memory_routines.h>
#include <launchdarkly/bindings/c/shared_function_argument_macro_definitions.h>
#include <launchdarkly/bindings/c/status.h>
#include <launchdarkly/bindings/c/value.h>

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDServerSDK* LDServerSDK;

/**
 * Constructs a new server-side LaunchDarkly SDK from a configuration.
 *
 * @param config The configuration. Ownership is transferred. Do not free or
 * access the LDServerConfig in any way after this call, otherwise behavior is
 * undefined. Must not be NULL.
 * @return New SDK instance. Must be freed with LDServerSDK_Free when no longer
 * needed.
 */
LD_EXPORT(LDServerSDK)
LDServerSDK_New(LDServerConfig config);

/**
 * Returns the version of the SDK.
 * @return String representation of the SDK version.
 */
LD_EXPORT(char const*)
LDServerSDK_Version(void);

/**
* Starts the SDK, initiating a connection to LaunchDarkly if not offline.
*
* Only one Start call can be in progress at once; calling it
* concurrently invokes undefined behavior.
*
* The method may be blocking or asynchronous depending on the arguments.
*
* To block, pass a positive milliseconds value and an optional pointer to a
boolean. The return
* value will be true if the SDK started within the specified timeframe, or
false if the
* operation couldn't complete in time. The value of out_succeeded will be true
* if the SDK successfully initialized.
*
* Example:
* @code
* bool initialized_successfully;
* if (LDServerSDK_Start(client, 5000, &initialized_successfully)) {
*     // The client was able to initialize in less than 5 seconds.
*     if (initialized_successfully) {
*         // Initialization succeeded.
*     else {
*         // Initialization failed.
*     }
* } else {
*    // The client is still initializing.
* }
* @endcode
*
* To start asynchronously, pass `LD_NONBLOCKING`. In this case, the return
value
* will be false and you may pass NULL to out_succeeded.
*
* @code
* // Returns immediately.
* LDServerSDK_Start(client, LD_NONBLOCKING, NULL);
* @endcode
*
* @param sdk SDK. Must not be NULL.
* @param milliseconds Milliseconds to wait for initialization or
`LD_NONBLOCKING` to return immediately.
* @param out_succeeded Pointer to bool representing successful initialization.
Only
* modified if a positive milliseconds value is passed; may be NULL.
* @return True if the client started within the specified timeframe.
*/
LD_EXPORT(bool)
LDServerSDK_Start(LDServerSDK sdk,
                  unsigned int milliseconds,
                  bool* out_succeeded);

/**
 * Returns a boolean value indicating LaunchDarkly connection and flag state
 * within the client.
 *
 * When you first start the client, once Start has completed, Initialized
 * should return true if and only if either 1. it connected to LaunchDarkly and
 * successfully retrieved flags, or 2. it started in offline mode so there's no
 * need to connect to LaunchDarkly.
 *
 * If the client timed out trying to connect to
 * LD, then Initialized returns false (even if we do have cached flags). If the
 * client connected and got a 401 error, Initialized is will return false. This
 * serves the purpose of letting the app know that there was a problem of some
 * kind.
 *
 * @param sdk SDK. Must not be NULL.
 * @return True if initialized.
 */
LD_EXPORT(bool)
LDServerSDK_Initialized(LDServerSDK sdk);

/**
 * Tracks that the given context performed an event with the given event name.
 * @param sdk SDK. Must not be NULL.
 * @param context The context. Ownership is NOT transferred. Must not be NULL.
 * @param event_name Name of the event. Must not be NULL.
 */
LD_EXPORT(void)
LDServerSDK_TrackEvent(LDServerSDK sdk,
                       LDContext context,
                       char const* event_name);

/**
 * Tracks that the given context performed an event with the given event
 * name, and associates it with a numeric metric and value.
 *
 * @param sdk SDK. Must not be NULL.
 * @param context The context. Ownership is NOT transferred. Must not be NULL.
 * @param event_name The name of the event. Must not be NULL.
 * @param metric_value This value is used by the LaunchDarkly experimentation
 * feature in numeric custom metrics, and will also be returned as part of the
 * custom event for Data Export.
 * @param data A JSON value containing additional data associated with the
 * event. Ownership is transferred into the SDK. Must not be NULL.
 */
LD_EXPORT(void)
LDServerSDK_TrackMetric(LDServerSDK sdk,
                        LDContext context,
                        char const* event_name,
                        double metric_value,
                        LDValue data);

/**
 * Tracks that the given context performed an event with the given event
 * name, with additional JSON data.
 * @param sdk SDK. Must not be NULL.
 * @param context The context. Ownership is NOT transferred. Must not be NULL.
 * @param event_name The name of the event. Must not be NULL.
 * @param data A JSON value containing additional data associated with the
 * event. Ownership is transferred. Must not be NULL.
 */
LD_EXPORT(void)
LDServerSDK_TrackData(LDServerSDK sdk,
                      LDContext context,
                      char const* event_name,
                      LDValue data);

/**
 * Requests delivery of all pending analytic events (if any).
 *
 * You MUST pass `LD_NONBLOCKING` as the second parameter.
 *
 * @param sdk SDK. Must not be NULL.
 * @param milliseconds Must pass `LD_NONBLOCKING`.
 */
LD_EXPORT(void)
LDServerSDK_Flush(LDServerSDK sdk, unsigned int reserved);

/**
 * Generates an identify event for the given context.
 *
 * @param sdk SDK. Must not be NULL.
 * @param context The context. Ownership is NOT transferred. Must not be NULL.
 */
LD_EXPORT(void)
LDServerSDK_Identify(LDServerSDK sdk, LDContext context);

/**
 * Returns the boolean value of a feature flag for a given flag key and context.
 * @param sdk SDK. Must not be NULL.
 * @param context The context. Ownership is NOT transferred. Must not be NULL.
 * @param flag_key The unique key for the feature flag. Must not be NULL.
 * @param default_value The default value of the flag.
 * @return The variation for the given context, or default_value if the
 * flag is disabled in the LaunchDarkly control panel.
 */
LD_EXPORT(bool)
LDServerSDK_BoolVariation(LDServerSDK sdk,
                          LDContext context,
                          char const* flag_key,
                          bool default_value);

/**
 * Returns the boolean value of a feature flag for a given flag key and context,
 * and details that also describes the way the value was determined.
 * @param sdk SDK. Must not be NULL.
 * @param context The context. Ownership is NOT transferred. Must not be NULL.
 * @param flag_key The unique key for the feature flag. Must not be NULL.
 * @param default_value The default value of the flag.
 * @param detail Out parameter to store the details. May pass LD_DISCARD_DETAILS
 * or NULL to discard the details. The details object must be freed with
 * LDEvalDetail_Free.
 * @return The variation for the given context, or default_value if the
 * flag is disabled in the LaunchDarkly control panel.
 */
LD_EXPORT(bool)
LDServerSDK_BoolVariationDetail(LDServerSDK sdk,
                                LDContext context,
                                char const* flag_key,
                                bool default_value,
                                LDEvalDetail* out_detail);

/**
 * Returns the string value of a feature flag for a given flag key and context.
 * Ensure the string is freed with LDMemory_FreeString.
 * @param sdk SDK. Must not be NULL.
 * @param context The context. Ownership is NOT transferred. Must not be NULL.
 * @param flag_key The unique key for the feature flag. Must not be NULL.
 * @param default_value The default value of the flag.
 * @return The variation for the given context, or a copy of default_value if
 * the flag is disabled in the LaunchDarkly control panel. Must be freed with
 * LDMemory_FreeString.
 */
LD_EXPORT(char*)
LDServerSDK_StringVariation(LDServerSDK sdk,
                            LDContext context,
                            char const* flag_key,
                            char const* default_value);

/**
 * Returns the string value of a feature flag for a given flag key and context,
 * and details that also describes the way the value was determined. Ensure the
 * string is freed with LDMemory_FreeString.
 * @param sdk SDK. Must not be NULL.
 * @param context The context. Ownership is NOT transferred. Must not be NULL.
 * @param flag_key The unique key for the feature flag. Must not be NULL.
 * @param default_value The default value of the flag.
 * @param detail Out parameter to store the details. May pass LD_DISCARD_DETAILS
 * or NULL to discard the details. The details object must be freed with
 * LDEvalDetail_Free.
 * @return The variation for the given context, or a copy of default_value if
 * the flag is disabled in the LaunchDarkly control panel. Must be freed with
 * LDMemory_FreeString.
 */
LD_EXPORT(char*)
LDServerSDK_StringVariationDetail(LDServerSDK sdk,
                                  LDContext context,
                                  char const* flag_key,
                                  char const* default_value,
                                  LDEvalDetail* out_detail);

/**
 * Returns the int value of a feature flag for a given flag key and context.
 * @param sdk SDK. Must not be NULL.
 * @param context The context. Ownership is NOT transferred. Must not be NULL.
 * @param flag_key The unique key for the feature flag. Must not be NULL.
 * @param default_value The default value of the flag.
 * @return The variation for the given context, or default_value if the
 * flag is disabled in the LaunchDarkly control panel.
 */
LD_EXPORT(int)
LDServerSDK_IntVariation(LDServerSDK sdk,
                         LDContext context,
                         char const* flag_key,
                         int default_value);

/**
 * Returns the int value of a feature flag for a given flag key and context, and
 * details that also describes the way the value was determined.
 * @param sdk SDK. Must not be NULL.
 * @param context The context. Ownership is NOT transferred. Must not be NULL.
 * @param flag_key The unique key for the feature flag. Must not be NULL.
 * @param default_value The default value of the flag.
 * @param detail Out parameter to store the details. May pass LD_DISCARD_DETAILS
 * or NULL to discard the details. The details object must be freed with
 * LDEvalDetail_Free.
 * @return The variation for the given context, or default_value if the
 * flag is disabled in the LaunchDarkly control panel.
 */
LD_EXPORT(int)
LDServerSDK_IntVariationDetail(LDServerSDK sdk,
                               LDContext context,
                               char const* flag_key,
                               int default_value,
                               LDEvalDetail* out_detail);

/**
 * Returns the double value of a feature flag for a given flag key and context.
 * @param sdk SDK. Must not be NULL.
 * @param context The context. Ownership is NOT transferred. Must not be NULL.
 * @param flag_key The unique key for the feature flag. Must not be NULL.
 * @param default_value The default value of the flag.
 * @return The variation for the given context, or default_value if the
 * flag is disabled in the LaunchDarkly control panel.
 */
LD_EXPORT(double)
LDServerSDK_DoubleVariation(LDServerSDK sdk,
                            LDContext context,
                            char const* flag_key,
                            double default_value);

/**
 * Returns the double value of a feature flag for a given flag key and context,
 * and details that also describes the way the value was determined.
 * @param sdk SDK. Must not be NULL.
 * @param context The context. Ownership is NOT transferred. Must not be NULL.
 * @param flag_key The unique key for the feature flag. Must not be NULL.
 * @param default_value The default value of the flag.
 * @param detail Out parameter to store the details. May pass LD_DISCARD_DETAILS
 * or NULL to discard the details. The details object must be freed with
 * LDEvalDetail_Free.
 * @return The variation for the given context, or default_value if the
 * flag is disabled in the LaunchDarkly control panel.
 */
LD_EXPORT(double)
LDServerSDK_DoubleVariationDetail(LDServerSDK sdk,
                                  LDContext context,
                                  char const* flag_key,
                                  double default_value,
                                  LDEvalDetail* out_detail);

/**
 * Returns the JSON value of a feature flag for a given flag key and context.
 * @param sdk SDK. Must not be NULL.
 * @param context The context. Ownership is NOT transferred. Must not be NULL.
 * @param flag_key The unique key for the feature flag. Must not be NULL.
 * @param default_value The default value of the flag. Ownership is NOT
 * transferred.
 * @return The variation for the given context, or a copy of default_value if
 * the flag is disabled in the LaunchDarkly control panel. The returned value
 * must be freed using LDValue_Free.
 */
LD_EXPORT(LDValue)
LDServerSDK_JsonVariation(LDServerSDK sdk,
                          LDContext context,
                          char const* flag_key,
                          LDValue default_value);

/**
 * Returns the JSON value of a feature flag for a given flag key and context,
 * and details that also describes the way the value was determined.
 * @param sdk SDK. Must not be NULL.
 * @param context The context. Ownership is NOT transferred. Must not be NULL.
 * @param flag_key The unique key for the feature flag. Must not be NULL.
 * @param default_value The default value of the flag. Ownership is NOT
 * transferred.
 * @param detail Out parameter to store the details. May pass LD_DISCARD_DETAILS
 * or NULL to discard the details. The details object must be freed with
 * LDEvalDetail_Free.
 * @return The variation for the given context, or a copy of default_value if
 * the flag is disabled in the LaunchDarkly control panel. The returned value
 * must be freed using LDValue_Free.
 */
LD_EXPORT(LDValue)
LDServerSDK_JsonVariationDetail(LDServerSDK sdk,
                                LDContext context,
                                char const* flag_key,
                                LDValue default_value,
                                LDEvalDetail* out_detail);

/**
 * Evaluates all flags for a context, returning a data structure containing
 * the results and additional flag metadata.
 *
 * The method's behavior can be controlled by passing a combination of
 * one or more options.
 *
 * A common use-case for AllFlagsState is to generate data suitable for
 * bootstrapping the client-side JavaScript SDK.
 *
 * This method will not send analytics events back to LaunchDarkly.
 *
 * @param sdk SDK. Must not be NULL.
 * @param context The context against which all flags will be evaluated.
 * Ownership is NOT transferred. Must not be NULL.
 * @param options A combination of one or more options. Pass
 * LD_ALLFLAGSSTATE_DEFAULT for default behavior.
 * @return An AllFlagsState data structure. Must be freed with
 * LDAllFlagsState_Free.
 */
LD_EXPORT(LDAllFlagsState)
LDServerSDK_AllFlagsState(LDServerSDK sdk,
                          LDContext context,
                          enum LDAllFlagsState_Options options);

/**
 * Frees the SDK's resources, shutting down any connections. May block.
 * @param sdk SDK.
 */
LD_EXPORT(void)
LDServerSDK_Free(LDServerSDK sdk);

typedef struct _LDServerDataSourceStatus* LDServerDataSourceStatus;

/**
 * Enumeration of possible data source states.
 */
enum LDServerDataSourceStatus_State {
    /**
     * The initial state of the data source when the SDK is being
     * initialized.
     *
     * If it encounters an error that requires it to retry initialization,
     * the state will remain at kInitializing until it either succeeds and
     * becomes LD_SERVERDATASOURCESTATUS_STATE_VALID, or permanently fails and
     * becomes LD_SERVERDATASOURCESTATUS_STATE_SHUTDOWN.
     */
    LD_SERVERDATASOURCESTATUS_STATE_INITIALIZING = 0,

    /**
     * Indicates that the data source is currently operational and has not
     * had any problems since the last time it received data.
     *
     * In streaming mode, this means that there is currently an open stream
     * connection and that at least one initial message has been received on
     * the stream. In polling mode, it means that the last poll request
     * succeeded.
     */
    LD_SERVERDATASOURCESTATUS_STATE_VALID = 1,

    /**
     * Indicates that the data source encountered an error that it will
     * attempt to recover from.
     *
     * In streaming mode, this means that the stream connection failed, or
     * had to be dropped due to some other error, and will be retried after
     * a backoff delay. In polling mode, it means that the last poll request
     * failed, and a new poll request will be made after the configured
     * polling interval.
     */
    LD_SERVERDATASOURCESTATUS_STATE_INTERRUPTED = 2,

    /**
     * Indicates that the data source has been permanently shut down.
     *
     * This could be because it encountered an unrecoverable error (for
     * instance, the LaunchDarkly service rejected the SDK key; an invalid
     * SDK key will never become valid), or because the SDK client was
     * explicitly shut down.
     */
    LD_SERVERDATASOURCESTATUS_STATE_OFF = 3
};

/**
 * Get an enumerated value representing the overall current state of the data
 * source.
 */
LD_EXPORT(enum LDServerDataSourceStatus_State)
LDServerDataSourceStatus_GetState(LDServerDataSourceStatus status);

/**
 * Information about the last error that the data source encountered, if
 * any. If there has not been an error, then NULL will be returned.
 *
 * If a non-NULL value is returned, then it should be freed using
 * LDDataSourceStatus_ErrorInfo_Free.
 *
 * This property should be updated whenever the data source encounters a
 * problem, even if it does not cause the state to change. For instance, if
 * a stream connection fails and the state changes to
 * LD_SERVERDATASOURCESTATUS_STATE_INTERRUPTED, and then subsequent attempts to
 * restart the connection also fail, the state will remain
 * LD_SERVERDATASOURCESTATUS_STATE_INTERRUPTED but the error information will be
 * updated each time-- and the last error will still be reported in this
 * property even if the state later becomes
 * LD_SERVERDATASOURCESTATUS_STATE_VALID.
 */
LD_EXPORT(LDDataSourceStatus_ErrorInfo)
LDServerDataSourceStatus_GetLastError(LDServerDataSourceStatus status);

/**
 * The date/time that the value of State most recently changed, in seconds
 * since epoch.
 *
 * The meaning of this depends on the current state:
 * - For LD_SERVERDATASOURCESTATUS_STATE_INITIALIZING, it is the time that the
 * SDK started initializing.
 * - For LD_SERVERDATASOURCESTATUS_STATE_VALID, it is the time that the data
 * source most recently entered a valid state, after previously having been
 * LD_SERVERDATASOURCESTATUS_STATE_INITIALIZING or an invalid state such as
 * LD_SERVERDATASOURCESTATUS_STATE_INTERRUPTED.
 * - For LD_SERVERDATASOURCESTATUS_STATE_INTERRUPTED, it is the time that the
 * data source most recently entered an error state, after previously having
 * been DataSourceState::kValid.
 * - For LD_SERVERDATASOURCESTATUS_STATE_SHUTDOWN, it is the time that the data
 * source encountered an unrecoverable error or that the SDK was explicitly shut
 * down.
 */
LD_EXPORT(time_t)
LDServerDataSourceStatus_StateSince(LDServerDataSourceStatus status);

typedef void (*ServerDataSourceStatusCallbackFn)(
    LDServerDataSourceStatus status,
    void* user_data);

/**
 * Defines a data source status listener which may be used to listen for
 * changes to the data source status.
 * The struct should be initialized using LDServerDataSourceStatusListener_Init
 * before use.
 */
struct LDServerDataSourceStatusListener {
    /**
     * Callback function which is invoked for data source status changes.
     *
     * The provided pointers are only valid for the duration of the function
     * call (excluding UserData, whose lifetime is controlled by the caller).
     *
     * @param status The updated data source status.
     */
    ServerDataSourceStatusCallbackFn StatusChanged;

    /**
     * UserData is forwarded into callback functions.
     */
    void* UserData;
};

/**
 * Initializes a data source status change listener. Must be called before
 * passing the listener to LDServerSDK_DataSourceStatus_OnStatusChange.
 *
 * If the StatusChanged member of the listener struct is not set (NULL), then
 * the function will not register a listener. In that case the return value
 * will be NULL.
 *
 * Create the struct, initialize the struct, set the StatusChanged handler
 * and optionally UserData, and then pass the struct to
 * LDServerSDK_DataSourceStatus_OnStatusChange. NULL will be returned if the
 * StatusChanged member of the listener struct is NULL.
 *
 * @param listener Listener to initialize.
 */
LD_EXPORT(void)
LDServerDataSourceStatusListener_Init(
    struct LDServerDataSourceStatusListener* listener);

/**
 * Listen for changes to the data source status.
 *
 * @param sdk SDK. Must not be NULL.
 * @param listener The listener, whose StatusChanged callback will be invoked,
 * when the data source status changes. Must not be NULL.
 *
 * @return A LDListenerConnection. The connection can be freed using
 * LDListenerConnection_Free and the listener can be disconnected using
 * LDListenerConnection_Disconnect.
 */
LD_EXPORT(LDListenerConnection)
LDServerSDK_DataSourceStatus_OnStatusChange(
    LDServerSDK sdk,
    struct LDServerDataSourceStatusListener listener);

/**
 * The current status of the data source.
 *
 * The caller must free the returned value using LDServerDataSourceStatus_Free.
 */
LD_EXPORT(LDServerDataSourceStatus)
LDServerSDK_DataSourceStatus_Status(LDServerSDK sdk);

/**
 * Frees the data source status.
 * @param status The data source status to free.
 */
LD_EXPORT(void)
LDServerDataSourceStatus_Free(LDServerDataSourceStatus status);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
