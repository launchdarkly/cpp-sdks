/** @file sdk.h
 * @brief LaunchDarkly Server-side C Bindings.
 */
// NOLINTBEGIN modernize-use-using
#pragma once

#include <launchdarkly/bindings/c/config/config.h>
#include <launchdarkly/bindings/c/context.h>
#include <launchdarkly/bindings/c/data/evaluation_detail.h>
#include <launchdarkly/bindings/c/export.h>
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

typedef struct _LDServerSDK* LDServerSDK;

#define LD_NONBLOCKING 0
#define LD_DISCARD_DETAIL NULL

/**
 * Constructs a new server-side LaunchDarkly SDK from a configuration.
 *
 * @param config The configuration. Must not be NULL.
 * @return New SDK instance.
 */
LD_EXPORT(LDServerSDK)
LDServerSDK_New(LDClientConfig config);

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
LD_EXPORT(bool) LDServerSDK_Initialized(LDServerSDK sdk);

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
 * @param context The context. Ownership is NOT transferred.Must not be NULL.
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
LD_EXPORT(int)
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
LD_EXPORT(int)
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
 * Frees the SDK's resources, shutting down any connections. May block.
 * @param sdk SDK.
 */
LD_EXPORT(void) LDServerSDK_Free(LDServerSDK sdk);

typedef void (*FlagChangedCallbackFn)(char const* flag_key,
                                      LDValue new_value,
                                      LDValue old_value,
                                      bool deleted,
                                      void* user_data);

/**
 * Defines a feature flag listener which may be used to listen for flag changes.
 * The struct should be initialized using LDFlagListener_Init before use.
 */
struct LDFlagListener {
    /**
     * Callback function which is invoked for flag changes.
     *
     * The provided pointers are only valid for the duration of the function
     * call (excluding UserData, whose lifetime is controlled by the caller).
     *
     * @param flag_key The name of the flag that changed.
     * @param new_value The new value of the flag. If there was not an new
     * value, because the flag was deleted, then the LDValue will be of a null
     * type. Check the deleted parameter to see if a flag was deleted.
     * @param old_value The old value of the flag. If there was not an old
     * value, for instance a newly created flag, then the Value will be of a
     * null type.
     * @param deleted True if the flag has been deleted.
     */
    FlagChangedCallbackFn FlagChanged;

    /**
     * UserData is forwarded into callback functions.
     */
    void* UserData;
};

/**
 * Initializes a flag listener. Must be called before passing the listener
 * to LDServerSDK_FlagNotifier_OnFlagChange.
 *
 * Create the struct, initialize the struct, set the FlagChanged handler
 * and optionally UserData, and then pass the struct to
 * LDServerSDK_FlagNotifier_OnFlagChange.
 *
 * @param listener Listener to initialize.
 */
LD_EXPORT(void) LDFlagListener_Init(struct LDFlagListener listener);

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
LDServerSDK_FlagNotifier_OnFlagChange(LDServerSDK sdk,
                                      char const* flag_key,
                                      struct LDFlagListener listener);

typedef struct _LDDataSourceStatus* LDDataSourceStatus;
typedef struct _LDDataSourceStatus_ErrorInfo* LDDataSourceStatus_ErrorInfo;

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
 * A description of an error condition that the data source encountered.
 */
enum LDDataSourceStatus_ErrorKind {
    /**
     * An unexpected error, such as an uncaught exception, further
     * described by the error message.
     */
    LD_DATASOURCESTATUS_ERRORKIND_UNKNOWN = 0,

    /**
     * An I/O error such as a dropped connection.
     */
    LD_DATASOURCESTATUS_ERRORKIND_NETWORK_ERROR = 1,

    /**
     * The LaunchDarkly service returned an HTTP response with an error
     * status, available in the status code.
     */
    LD_DATASOURCESTATUS_ERRORKIND_ERROR_RESPONSE = 2,

    /**
     * The SDK received malformed data from the LaunchDarkly service.
     */
    LD_DATASOURCESTATUS_ERRORKIND_INVALID_DATA = 3,

    /**
     * The data source itself is working, but when it tried to put an
     * update into the data store, the data store failed (so the SDK may
     * not have the latest data).
     */
    LD_DATASOURCESTATUS_ERRORKIND_STORE_ERROR = 4,
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

/**
 * Get an enumerated value representing the general category of the error.
 */
LD_EXPORT(enum LDDataSourceStatus_ErrorKind)
LDDataSourceStatus_ErrorInfo_GetKind(LDDataSourceStatus_ErrorInfo info);

/**
 * The HTTP status code if the error was
 * LD_DATASOURCESTATUS_ERRORKIND_ERROR_RESPONSE.
 */
LD_EXPORT(uint64_t)
LDDataSourceStatus_ErrorInfo_StatusCode(LDDataSourceStatus_ErrorInfo info);

/**
 * Any additional human-readable information relevant to the error.
 *
 * The format is subject to change and should not be relied on
 * programmatically.
 */
LD_EXPORT(char const*)
LDDataSourceStatus_ErrorInfo_Message(LDDataSourceStatus_ErrorInfo info);

/**
 * The date/time that the error occurred, in seconds since epoch.
 */
LD_EXPORT(time_t)
LDDataSourceStatus_ErrorInfo_Time(LDDataSourceStatus_ErrorInfo info);

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
LDDataSourceStatusListener_Init(struct LDDataSourceStatusListener listener);

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
    struct LDDataSourceStatusListener listener);

/**
 * The current status of the data source.
 *
 * The caller must free the returned value using LDDataSourceStatus_Free.
 */
LD_EXPORT(LDDataSourceStatus)
LDServerSDK_DataSourceStatus_Status(LDServerSDK sdk);

/**
 * Frees the data source status.
 * @param status The data source status to free.
 */
LD_EXPORT(void) LDDataSourceStatus_Free(LDDataSourceStatus status);

/**
 * Frees the data source status error information.
 * @param status The error information to free.
 */
LD_EXPORT(void)
LDDataSourceStatus_ErrorInfo_Free(LDDataSourceStatus_ErrorInfo info);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
