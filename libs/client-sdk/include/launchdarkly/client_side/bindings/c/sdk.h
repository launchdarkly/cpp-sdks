// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/config/config.h>
#include <launchdarkly/bindings/c/context.h>
#include <launchdarkly/bindings/c/export.h>
#include <launchdarkly/bindings/c/status.h>
#include <launchdarkly/bindings/c/value.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDClientSDK* LDClientSDK;
typedef struct _LDClientSDK_EvalDetail* LDClientSDK_EvalDetail;

/**
 * Frees the detail structure optionally returned by *VariationDetail functions.
 * @param detail Detail to free.
 */
LD_EXPORT(void)
LDClientSDK_EvalDetail_Free(LDClientSDK_EvalDetail detail);

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
 * To block until the flush operation is complete or a timeout is reached,
 * pass a non-negative milliseconds parameter.
 *
 * NOTE: Current behavior is that Flush will return immediately regardless of
 * the milliseconds parameter but this may change in the future.
 *
 * @param sdk SDK. Must not be NULL.
 * @param milliseconds How long to wait for the flush to complete, or a negative
 * number to return immediately.
 */
LD_EXPORT(void)
LDClientSDK_Flush(LDClientSDK sdk, int milliseconds);

/**
 * Changes the current evaluation context, requests flags for that context
 * from LaunchDarkly if online, and generates an analytics event to
 * tell LaunchDarkly about the context.
 *
 * Only one Identify call can be in progress at once; calling it
 * concurrently invokes undefined behavior.
 *
 * To block until the identify operation is complete or a timeout is reached,
 * pass a non-negative milliseconds parameter.
 *
 * @param sdk SDK. Must not be NULL.
 * @param context The new evaluation context.
 * @param milliseconds How long to wait for the identify to complete, or a
 * negative number to return immediately.
 */
LD_EXPORT(void)
LDClientSDK_Identify(LDClientSDK sdk, LDContext context, int milliseconds);

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
 * @param detail Out parameter to store the details. May pass NULL to discard
 * the details. The details object must be freed with
 * LDClientSDK_EvalDetail_Free.
 * @return The variation for the current context, or default_value if the
 * flag is disabled in the LaunchDarkly control panel.
 */
LD_EXPORT(bool)
LDClientSDK_BoolVariationDetail(LDClientSDK sdk,
                                char const* flag_key,
                                bool default_value,
                                LDClientSDK_EvalDetail* detail);

/**
 * Frees the SDK's resources, shutting down any connections. May block.
 * @param sdk SDK.
 */
LD_EXPORT(void) LDClientSDK_Free(LDClientSDK sdk);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
