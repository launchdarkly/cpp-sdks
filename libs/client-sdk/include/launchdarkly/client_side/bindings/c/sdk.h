/** @file sdk.h
 * @brief LaunchDarkly Client-side C Bindings.
 */
// NOLINTBEGIN modernize-use-using
#pragma once

#include <launchdarkly/client_side/bindings/c/config/config.h>

#include <launchdarkly/bindings/c/context.h>
#include <launchdarkly/bindings/c/data/evaluation_detail.h>
#include <launchdarkly/bindings/c/data_source/error_info.h>
#include <launchdarkly/bindings/c/export.h>
#include <launchdarkly/bindings/c/flag_listener.h>
#include <launchdarkly/bindings/c/listener_connection.h>
#include <launchdarkly/bindings/c/memory_routines.h>
#include <launchdarkly/bindings/c/status.h>
#include <launchdarkly/bindings/c/value.h>

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDClientSDK* LDClientSDK;

#ifndef LD_NONBLOCKING
#define LD_NONBLOCKING 0
#endif
#ifndef LD_DISCARD_DETAIL
#define LD_DISCARD_DETAIL NULL
#endif

/**
 * Constructs a new client-side LaunchDarkly SDK from a configuration and
 * context.
 * @param config The configuration. Ownership is transferred. Do not free or
 * access the LDClientConfig in any way after this call; behavior is undefined.
 * Must not be NULL.
 * @param context The initial context. Ownership is transferred. Do not free or
 * access the LDContext in any way after this call; behavior is undefined. Must
 * not be NULL.
 * @return New SDK instance. Must be freed with LDClientSDK_Free when no longer
 * needed.
 */
LD_EXPORT(LDClientSDK)
LDClientSDK_New(LDClientConfig config, LDContext context);

/**
 * Returns the version of the SDK.
 * @return String representation of the SDK version.
 */
LD_EXPORT(char const*)
LDClientSDK_Version(void);

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
 * if (LDClientSDK_Start(client, 5000, &initialized_successfully)) {
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
 * LDClientSDK_Start(client, LD_NONBLOCKING, NULL);
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
LDClientSDK_Start(LDClientSDK sdk,
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
LD_EXPORT(bool) LDClientSDK_Initialized(LDClientSDK sdk);

/**
 * Tracks that the current context performed an event for the given event name.
 * @param sdk SDK. Must not be NULL.
 * @param event_name Name of the event. Must not be NULL.
 */
LD_EXPORT(void) LDClientSDK_TrackEvent(LDClientSDK sdk, char const* event_name);

/**
 * Tracks that the current context performed an event for the given event
 * name, and associates it with a numeric metric and value.
 *
 * @param sdk SDK. Must not be NULL.
 * @param event_name The name of the event. Must not be NULL.
 * @param metric_value this value is used by the LaunchDarkly experimentation
 * feature in numeric custom metrics, and will also be returned as part of the
 * custom event for Data Export.
 * @param data A JSON value containing additional data associated with the
 * event. Ownership is transferred. Do not free or
 * access the LDValue in any way after this call; behavior is undefined. Must
 * not be NULL.
 */
LD_EXPORT(void)
LDClientSDK_TrackMetric(LDClientSDK sdk,
                        char const* event_name,
                        double metric_value,
                        LDValue data);

/**
 * Tracks that the current context performed an event for the given event
 * name, with additional JSON data.
 * @param sdk SDK. Must not be NULL.
 * @param event_name Must not be NULL.
 * @param data A JSON value containing additional data associated with the
 * event. Do not free or
 * access the LDValue in any way after this call; behavior is undefined. Must
 * not be NULL.
 */
LD_EXPORT(void)
LDClientSDK_TrackData(LDClientSDK sdk, char const* event_name, LDValue data);

/**
 * Requests delivery of all pending analytic events (if any).
 *
 * You MUST pass `LD_NONBLOCKING` as the second parameter.
 *
 * @param sdk SDK. Must not be NULL.
 * @param milliseconds Must pass `LD_NONBLOCKING`.
 */
LD_EXPORT(void)
LDClientSDK_Flush(LDClientSDK sdk, unsigned int reserved);

/**
 * Changes the current evaluation context, requests flags for that context
 * from LaunchDarkly if online, and generates an analytics event to
 * tell LaunchDarkly about the context.
 *
 * Only one Identify call can be in progress at once; calling it
 * concurrently invokes undefined behavior.
 *
 * The method may be blocking or asynchronous depending on the arguments.
 *
 * To block, pass a positive milliseconds value and an optional pointer to a
 boolean. The return
 * value will be true if the SDK was able to attempt the operation within
 the specified timeframe, or false if the
 * operation couldn't complete in time. The value of out_succeeded will be true
 * if the SDK successfully changed evaluation contexts.
 *
 * Example:
 * @code
 * bool identified_successfully;
 * if (LDClientSDK_Identify(client, context, 5000, &identified_successfully)) {
 *     // The client was able to re-initialize in less than 5 seconds.
 *     if (identified_successfully) {
 *         // Evaluations will use data for the new context.
 *     else {
 *         // Evaluations will continue using existing data.
 *     }
 * } else {
 *    // The client is still identifying.
 * }
 * @endcode
 *
 * To start asynchronously, pass `LD_NONBLOCKING`. In this case, the return
value
 * will be false and you may pass NULL to out_succeeded.
 *
 * @code
 * // Returns immediately.
 * LDClientSDK_Identify(client, context, LD_NONBLOCKING, NULL);
 * @endcode
 *
 *
 * @param sdk SDK. Must not be NULL.
 * @param context The new evaluation context.
 * @param milliseconds Milliseconds to wait for identify to complete, or
 * `LD_NONBLOCKING` to return immediately.
 * @param out_succeeded Pointer to bool representing successful identification.
Only
* modified if a positive milliseconds value is passed; may be NULL.
 * @return True if the client started within the specified timeframe.
 */
LD_EXPORT(bool)
LDClientSDK_Identify(LDClientSDK sdk,
                     LDContext context,
                     unsigned int milliseconds,
                     bool* out_succeeded);

/**
 * Returns the boolean value of a feature flag for a given flag key.
 * @param sdk SDK. Must not be NULL.
 * @param flag_key The unique key for the feature flag. Must not be NULL.
 * @param default_value The default value of the flag.
 * @return The variation for the current context, or default_value if the
 * flag is disabled in the LaunchDarkly control panel.
 */
LD_EXPORT(bool)
LDClientSDK_BoolVariation(LDClientSDK sdk,
                          char const* flag_key,
                          bool default_value);

/**
 * Returns the boolean value of a feature flag for a given flag key, and details
 * that also describes the way the value was determined.
 * @param sdk SDK. Must not be NULL.
 * @param flag_key The unique key for the feature flag. Must not be NULL.
 * @param default_value The default value of the flag.
 * @param detail Out parameter to store the details. May pass LD_DISCARD_DETAILS
 * or NULL to discard the details. The details object must be freed with
 * LDEvalDetail_Free.
 * @return The variation for the current context, or default_value if the
 * flag is disabled in the LaunchDarkly control panel.
 */
LD_EXPORT(bool)
LDClientSDK_BoolVariationDetail(LDClientSDK sdk,
                                char const* flag_key,
                                bool default_value,
                                LDEvalDetail* out_detail);

/**
 * Returns the string value of a feature flag for a given flag key. Ensure the
 * string is freed with LDMemory_FreeString.
 * @param sdk SDK. Must not be NULL.
 * @param flag_key The unique key for the feature flag. Must not be NULL.
 * @param default_value The default value of the flag.
 * @return The variation for the current context, or a copy of default_value if
 * the flag is disabled in the LaunchDarkly control panel. Must be freed with
 * LDMemory_FreeString.
 */
LD_EXPORT(char*)
LDClientSDK_StringVariation(LDClientSDK sdk,
                            char const* flag_key,
                            char const* default_value);

/**
 * Returns the string value of a feature flag for a given flag key, and details
 * that also describes the way the value was determined. Ensure the
 * string is freed with LDMemory_FreeString.
 * @param sdk SDK. Must not be NULL.
 * @param flag_key The unique key for the feature flag. Must not be NULL.
 * @param default_value The default value of the flag.
 * @param detail Out parameter to store the details. May pass LD_DISCARD_DETAILS
 * or NULL to discard the details. The details object must be freed with
 * LDEvalDetail_Free.
 * @return The variation for the current context, or a copy of default_value if
 * the flag is disabled in the LaunchDarkly control panel. Must be freed with
 * LDMemory_FreeString.
 */
LD_EXPORT(char*)
LDClientSDK_StringVariationDetail(LDClientSDK sdk,
                                  char const* flag_key,
                                  char const* default_value,
                                  LDEvalDetail* out_detail);

/**
 * Returns the int value of a feature flag for a given flag key.
 * @param sdk SDK. Must not be NULL.
 * @param flag_key The unique key for the feature flag. Must not be NULL.
 * @param default_value The default value of the flag.
 * @return The variation for the current context, or default_value if the
 * flag is disabled in the LaunchDarkly control panel.
 */
LD_EXPORT(int)
LDClientSDK_IntVariation(LDClientSDK sdk,
                         char const* flag_key,
                         int default_value);

/**
 * Returns the int value of a feature flag for a given flag key, and details
 * that also describes the way the value was determined.
 * @param sdk SDK. Must not be NULL.
 * @param flag_key The unique key for the feature flag. Must not be NULL.
 * @param default_value The default value of the flag.
 * @param detail Out parameter to store the details. May pass LD_DISCARD_DETAILS
 * or NULL to discard the details. The details object must be freed with
 * LDEvalDetail_Free.
 * @return The variation for the current context, or default_value if the
 * flag is disabled in the LaunchDarkly control panel.
 */
LD_EXPORT(int)
LDClientSDK_IntVariationDetail(LDClientSDK sdk,
                               char const* flag_key,
                               int default_value,
                               LDEvalDetail* out_detail);

/**
 * Returns the double value of a feature flag for a given flag key.
 * @param sdk SDK. Must not be NULL.
 * @param flag_key The unique key for the feature flag. Must not be NULL.
 * @param default_value The default value of the flag.
 * @return The variation for the current context, or default_value if the
 * flag is disabled in the LaunchDarkly control panel.
 */
LD_EXPORT(int)
LDClientSDK_DoubleVariation(LDClientSDK sdk,
                            char const* flag_key,
                            double default_value);

/**
 * Returns the double value of a feature flag for a given flag key, and details
 * that also describes the way the value was determined.
 * @param sdk SDK. Must not be NULL.
 * @param flag_key The unique key for the feature flag. Must not be NULL.
 * @param default_value The default value of the flag.
 * @param detail Out parameter to store the details. May pass LD_DISCARD_DETAILS
 * or NULL to discard the details. The details object must be freed with
 * LDEvalDetail_Free.
 * @return The variation for the current context, or default_value if the
 * flag is disabled in the LaunchDarkly control panel.
 */
LD_EXPORT(int)
LDClientSDK_DoubleVariationDetail(LDClientSDK sdk,
                                  char const* flag_key,
                                  double default_value,
                                  LDEvalDetail* out_detail);

/**
 * Returns the JSON value of a feature flag for a given flag key.
 * @param sdk SDK. Must not be NULL.
 * @param flag_key The unique key for the feature flag. Must not be NULL.
 * @param default_value The default value of the flag. Ownership is retained by
 * the caller; a copy is made internally. Must not be NULL.
 * @return The variation for the current context, or a copy of default_value if
 * the flag is disabled in the LaunchDarkly control panel. The returned value
 * must be freed using LDValue_Free.
 */
LD_EXPORT(LDValue)
LDClientSDK_JsonVariation(LDClientSDK sdk,
                          char const* flag_key,
                          LDValue default_value);

/**
 * Returns the JSON value of a feature flag for a given flag key, and details
 * that also describes the way the value was determined.
 * @param sdk SDK. Must not be NULL.
 * @param flag_key The unique key for the feature flag. Must not be NULL.
 * @param default_value The default value of the flag. Ownership is retained by
 * the caller; a copy is made internally. Must not be NULL.
 * @param detail Out parameter to store the details. May pass LD_DISCARD_DETAILS
 * or NULL to discard the details. The details object must be freed with
 * LDEvalDetail_Free.
 * @return The variation for the current context, or a copy of default_value if
 * the flag is disabled in the LaunchDarkly control panel. The returned value
 * must be freed using LDValue_Free.
 */
LD_EXPORT(LDValue)
LDClientSDK_JsonVariationDetail(LDClientSDK sdk,
                                char const* flag_key,
                                LDValue default_value,
                                LDEvalDetail* out_detail);

/**
 * Returns a map from feature flag keys to feature flag values for the current
 * context.
 *
 * In the example, all flags of type boolean are printed.
 * @code
 * LDValue all_flags = LDClientSDK_AllFlags(sdk);
 * LDValue_ObjectIter it;
 * for (it = LDValue_CreateObjectIter(all_flags);
 * !LDValue_ObjectIter_End(it); LDValue_ObjectIter_Next(it)) { char
 * const* flag_key = LDValue_ObjectIter_Key(it); LDValue flag_val_ref =
 * LDValue_ObjectIter_Value(it);
 *
 *   if (LDValue_Type(flag_val_ref) == LDValueType_Bool) {
 *       printf("%s: %d\n", flag_key, LDValue_GetBool(flag_val_ref));
 *   }
 * }
 * @endcode
 * @param sdk SDK. Must not be NULL.
 * @return Value of type Object. Must be freed with LDValue_Free.
 */
LD_EXPORT(LDValue)
LDClientSDK_AllFlags(LDClientSDK sdk);

/**
 * Frees the SDK's resources, shutting down any connections. May block.
 * @param sdk SDK.
 */
LD_EXPORT(void) LDClientSDK_Free(LDClientSDK sdk);

/**
 * Listen for changes for the specific flag.
 *
 * If the FlagChanged member of the listener struct is not set (NULL), then the
 * function will not register a listener. In that case the return value
 * will be NULL.
 *
 * @param sdk SDK. Must not be NULL.
 * @param flag_key The unique key for the feature flag. Must not be NULL.
 * @param listener The listener, whose FlagChanged callback will be invoked,
 * when the flag changes. Must not be NULL.
 *
 * @return A LDListenerConnection. The connection can be freed using
 * LDListenerConnection_Free and the listener can be disconnected using
 * LDListenerConnection_Disconnect. NULL will be returned if the FlagChanged
 * member of the listener struct is NULL.
 */
LD_EXPORT(LDListenerConnection)
LDClientSDK_FlagNotifier_OnFlagChange(LDClientSDK sdk,
                                      char const* flag_key,
                                      struct LDFlagListener listener);

typedef struct _LDDataSourceStatus* LDDataSourceStatus;

/**
 * Enumeration of possible data source states.
 */
enum LDDataSourceStatus_State {
    /**
     * The initial state of the data source when the SDK is being
     * initialized.
     *
     * If it encounters an error that requires it to retry initialization,
     * the state will remain at kInitializing until it either succeeds and
     * becomes LD_DATASOURCESTATUS_STATE_VALID, or permanently fails and becomes
     * LD_DATASOURCESTATUS_STATE_SHUTDOWN.
     */
    LD_DATASOURCESTATUS_STATE_INITIALIZING = 0,

    /**
     * Indicates that the data source is currently operational and has not
     * had any problems since the last time it received data.
     *
     * In streaming mode, this means that there is currently an open stream
     * connection and that at least one initial message has been received on
     * the stream. In polling mode, it means that the last poll request
     * succeeded.
     */
    LD_DATASOURCESTATUS_STATE_VALID = 1,

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
    LD_DATASOURCESTATUS_STATE_INTERRUPTED = 2,

    /**
     * Indicates that the application has told the SDK to stay offline.
     */
    LD_DATASOURCESTATUS_STATE_OFFLINE = 3,

    /**
     * Indicates that the data source has been permanently shut down.
     *
     * This could be because it encountered an unrecoverable error (for
     * instance, the LaunchDarkly service rejected the SDK key; an invalid
     * SDK key will never become valid), or because the SDK client was
     * explicitly shut down.
     */
    LD_DATASOURCESTATUS_STATE_SHUTDOWN = 4
};

/**
 * Get an enumerated value representing the overall current state of the data
 * source.
 */
LD_EXPORT(enum LDDataSourceStatus_State)
LDDataSourceStatus_GetState(LDDataSourceStatus status);

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
 * LD_DATASOURCESTATUS_STATE_INTERRUPTED, and then subsequent attempts to
 * restart the connection also fail, the state will remain
 * LD_DATASOURCESTATUS_STATE_INTERRUPTED but the error information will be
 * updated each time-- and the last error will still be reported in this
 * property even if the state later becomes LD_DATASOURCESTATUS_STATE_VALID.
 */
LD_EXPORT(LDDataSourceStatus_ErrorInfo)
LDDataSourceStatus_GetLastError(LDDataSourceStatus status);

/**
 * The date/time that the value of State most recently changed, in seconds
 * since epoch.
 *
 * The meaning of this depends on the current state:
 * - For LD_DATASOURCESTATUS_STATE_INITIALIZING, it is the time that the SDK
 * started initializing.
 * - For LD_DATASOURCESTATUS_STATE_VALID, it is the time that the data
 * source most recently entered a valid state, after previously having been
 * LD_DATASOURCESTATUS_STATE_INITIALIZING or an invalid state such as
 * LD_DATASOURCESTATUS_STATE_INTERRUPTED.
 * - For LD_DATASOURCESTATUS_STATE_INTERRUPTED, it is the time that the data
 * source most recently entered an error state, after previously having been
 * DataSourceState::kValid.
 * - For LD_DATASOURCESTATUS_STATE_SHUTDOWN, it is the time that the data source
 * encountered an unrecoverable error or that the SDK was explicitly shut
 * down.
 */
LD_EXPORT(time_t) LDDataSourceStatus_StateSince(LDDataSourceStatus status);

typedef void (*DataSourceStatusCallbackFn)(LDDataSourceStatus status,
                                           void* user_data);

/**
 * Defines a data source status listener which may be used to listen for
 * changes to the data source status.
 * The struct should be initialized using LDDataSourceStatusListener_Init
 * before use.
 */
struct LDDataSourceStatusListener {
    /**
     * Callback function which is invoked for data source status changes.
     *
     * The provided pointers are only valid for the duration of the function
     * call (excluding UserData, whose lifetime is controlled by the caller).
     *
     * @param status The updated data source status.
     */
    DataSourceStatusCallbackFn StatusChanged;

    /**
     * UserData is forwarded into callback functions.
     */
    void* UserData;
};

/**
 * Initializes a data source status change listener. Must be called before
 * passing the listener to LDClientSDK_DataSourceStatus_OnStatusChange.
 *
 * If the StatusChanged member of the listener struct is not set (NULL), then
 * the function will not register a listener. In that case the return value
 * will be NULL.
 *
 * Create the struct, initialize the struct, set the StatusChanged handler
 * and optionally UserData, and then pass the struct to
 * LDClientSDK_DataSourceStatus_OnStatusChange. NULL will be returned if the
 * StatusChanged member of the listener struct is NULL.
 *
 * @param listener Listener to initialize.
 */
LD_EXPORT(void)
LDDataSourceStatusListener_Init(struct LDDataSourceStatusListener* listener);

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
LDClientSDK_DataSourceStatus_OnStatusChange(
    LDClientSDK sdk,
    struct LDDataSourceStatusListener listener);

/**
 * The current status of the data source.
 *
 * The caller must free the returned value using LDDataSourceStatus_Free.
 */
LD_EXPORT(LDDataSourceStatus)
LDClientSDK_DataSourceStatus_Status(LDClientSDK sdk);

/**
 * Frees the data source status.
 * @param status The data source status to free.
 */
LD_EXPORT(void) LDDataSourceStatus_Free(LDDataSourceStatus status);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
