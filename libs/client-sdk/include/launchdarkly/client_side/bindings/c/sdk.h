// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/config/config.h>
#include <launchdarkly/bindings/c/context.h>
#include <launchdarkly/bindings/c/data/evaluation_detail.h>
#include <launchdarkly/bindings/c/export.h>
#include <launchdarkly/bindings/c/memory_routines.h>
#include <launchdarkly/bindings/c/status.h>
#include <launchdarkly/bindings/c/value.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDClientSDK* LDClientSDK;

#define LD_NONBLOCKING 0
#define LD_DISCARD_DETAIL NULL

/**
 * Constructs a new client-side LaunchDarkly SDK from a configuration and
 * context.
 * @param config The configuration. Must not be NULL.
 * @param context The initial context. Must not be NULL.
 * @return New SDK instance.
 */
LD_EXPORT(LDClientSDK)
LDClientSDK_New(LDClientConfig config, LDContext context);

/**
 * Returns a boolean value indicating LaunchDarkly connection and flag state
 * within the client.
 *
 *  [TODO Need to make WaitForReadyAsync, offline]
 * When you first start the client, once WaitForReadySync has returned or
 * WaitForReadyAsync has completed, Initialized should return true if
 * and only if either 1. it connected to LaunchDarkly and successfully
 * retrieved flags, or 2. it started in offline mode so there's no need to
 * connect to LaunchDarkly. If the client timed out trying to connect to LD,
 * then Initialized returns false (even if we do have cached flags).
 * If the client connected and got a 401 error, Initialized is
 * will return false. This serves the purpose of letting the app know that
 * there was a problem of some kind.
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
 * event. Must not be NULL.
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
 * event. Must not be NULL.
 */
LD_EXPORT(void)
LDClientSDK_TrackData(LDClientSDK sdk, char const* event_name, LDValue data);

/**
 * Requests delivery of all pending analytic events (if any).
 *
 * You MUST pass LD_NONBLOCKING as the second parameter.
 *
 * @param sdk SDK. Must not be NULL.
 * @param milliseconds Must pass LD_NONBLOCKING.
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
 * To block until the identify operation is complete or a timeout is reached,
 * pass a positive milliseconds parameter. Otherwise to return immediately,
 * pass LD_NONBLOCKING.
 *
 * @param sdk SDK. Must not be NULL.
 * @param context The new evaluation context.
 * @param milliseconds How long to wait for the identify to complete, or
 * LD_NONBLOCKING to return immediately.
 */
LD_EXPORT(void)
LDClientSDK_Identify(LDClientSDK sdk,
                     LDContext context,
                     unsigned int milliseconds);

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
 * @param default_value The default value of the flag. The value is copied.
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
 * @param default_value The default value of the flag. The value is copied.
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
 * Frees the SDK's resources, shutting down any connections. May block.
 * @param sdk SDK.
 */
LD_EXPORT(void) LDClientSDK_Free(LDClientSDK sdk);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
