/**
 * @file track_series_context.h
 * @brief C bindings for read-only track context passed to afterTrack hooks.
 *
 * TrackSeriesContext provides information about a track event to hook callbacks.
 * Most data is read-only and valid only during the callback execution.
 *
 * LIFETIME AND OWNERSHIP:
 * - All context parameters are temporary - valid only during callback
 * - Do not store pointers to context, key, or environment ID strings
 * - Data retrieved with LDTrackSeriesContext_Data() is temporary - valid only
 *   during callback
 * - Do not call LDValue_Free() on data retrieved with LDTrackSeriesContext_Data()
 * - Metric values and hook context are temporary - valid only during callback
 */

#pragma once

#include <launchdarkly/bindings/c/export.h>
#include <launchdarkly/server_side/bindings/c/hook_context.h>
#include <launchdarkly/bindings/c/value.h>
#include <launchdarkly/bindings/c/context.h>

// No effect in C++, but we want it for C.
// ReSharper disable once CppUnusedIncludeDirective
#include <stdbool.h> // NOLINT(*-deprecated-headers)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct p_LDServerSDKTrackSeriesContext* LDServerSDKTrackSeriesContext;

/**
 * @brief Get the event key for the track call.
 *
 * @param track_context Track context. Must not be NULL.
 * @return Event key as null-terminated UTF-8 string. Valid only during
 *         the callback execution. Must not be freed.
 */
LD_EXPORT(char const*)
LDTrackSeriesContext_Key(LDServerSDKTrackSeriesContext track_context);

/**
 * @brief Get the context (user/organization) associated with the track call.
 *
 * @param track_context Track context. Must not be NULL.
 * @return Context object. Valid only during the callback execution.
 *         Must not be freed. Do not call LDContext_Free() on this.
 */
LD_EXPORT(LDContext)
LDTrackSeriesContext_Context(LDServerSDKTrackSeriesContext track_context);

/**
 * @brief Get the application-specified data for the track call, if any.
 *
 * LIFETIME: Returns a temporary value valid only during the callback.
 * Do not call LDValue_Free() on the returned value.
 *
 * USAGE:
 * @code
 *   LDValue data;
 *   if (LDTrackSeriesContext_Data(track_context, &data)) {
 *       // Use data (valid only during callback)
 *       char const* str = LDValue_GetString(data);
 *       // Do NOT call LDValue_Free(data)
 *   }
 * @endcode
 *
 * @param track_context Track context. Must not be NULL.
 * @param out_data Pointer to receive the data value. Must not be NULL.
 *                 Set to a temporary LDValue (valid only during callback).
 *                 Do not call LDValue_Free() on this value. Set to NULL if no
 *                 data was provided.
 * @return true if data was provided, false if no data.
 */
LD_EXPORT(bool)
LDTrackSeriesContext_Data(LDServerSDKTrackSeriesContext track_context, LDValue* out_data);

/**
 * @brief Get the metric value for the track call, if any.
 *
 * @param track_context Track context. Must not be NULL.
 * @param out_metric_value Pointer to receive the metric value. Must not be
 *                         NULL. Only set if a metric value was provided.
 * @return true if a metric value was provided, false otherwise.
 */
LD_EXPORT(bool)
LDTrackSeriesContext_MetricValue(LDServerSDKTrackSeriesContext track_context,
                                  double* out_metric_value);

/**
 * @brief Get the hook context provided by the caller.
 *
 * This contains application-specific data passed to the track call,
 * such as OpenTelemetry span parents.
 *
 * @param track_context Track context. Must not be NULL.
 * @return Hook context. Valid only during the callback execution.
 *         Must not be freed. Do not call LDHookContext_Free() on this.
 */
LD_EXPORT(LDHookContext)
LDTrackSeriesContext_HookContext(LDServerSDKTrackSeriesContext track_context);

/**
 * @brief Get the environment ID, if available.
 *
 * The environment ID is only available after SDK initialization completes.
 * Returns NULL if not yet available.
 *
 * @param track_context Track context. Must not be NULL.
 * @return Environment ID as null-terminated UTF-8 string, or NULL if not
 *         available. Valid only during the callback execution. Must not
 *         be freed.
 */
LD_EXPORT(char const*)
LDTrackSeriesContext_EnvironmentId(LDServerSDKTrackSeriesContext track_context);

#ifdef __cplusplus
}
#endif
