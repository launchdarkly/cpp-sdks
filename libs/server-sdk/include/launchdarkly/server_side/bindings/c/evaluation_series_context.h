/**
 * @file evaluation_series_context.h
 * @brief C bindings for read-only evaluation context passed to hooks.
 *
 * EvaluationSeriesContext provides information about a flag evaluation
 * to hook callbacks. All data is read-only and valid only during the
 * callback execution.
 *
 * LIFETIME:
 * - All context parameters are temporary - valid only during callback
 * - Do not store pointers to context data
 * - Copy any needed data (strings, values) if you need to retain it
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

typedef struct p_LDServerSDKEvaluationSeriesContext* LDServerSDKEvaluationSeriesContext;

/**
 * @brief Get the flag key being evaluated.
 *
 * @param eval_context Evaluation context. Must not be NULL.
 * @return Flag key as null-terminated UTF-8 string. Valid only during
 *         the callback execution. Must not be freed.
 */
LD_EXPORT(char const*)
LDEvaluationSeriesContext_FlagKey(LDServerSDKEvaluationSeriesContext eval_context);

/**
 * @brief Get the context (user/organization) being evaluated.
 *
 * @param eval_context Evaluation context. Must not be NULL.
 * @return Context object. Valid only during the callback execution.
 *         Must not be freed. Do not call LDContext_Free() on this.
 */
LD_EXPORT(LDContext)
LDEvaluationSeriesContext_Context(LDServerSDKEvaluationSeriesContext eval_context);

/**
 * @brief Get the default value provided to the variation call.
 *
 * @param eval_context Evaluation context. Must not be NULL.
 * @return Default value. Valid only during the callback execution.
 *         Must not be freed. Do not call LDValue_Free() on this.
 */
LD_EXPORT(LDValue)
LDEvaluationSeriesContext_DefaultValue(
    LDServerSDKEvaluationSeriesContext eval_context);

/**
 * @brief Get the name of the variation method called.
 *
 * Examples: "BoolVariation", "StringVariationDetail", "JsonVariation"
 *
 * @param eval_context Evaluation context. Must not be NULL.
 * @return Method name as null-terminated UTF-8 string. Valid only during
 *         the callback execution. Must not be freed.
 */
LD_EXPORT(char const*)
LDEvaluationSeriesContext_Method(LDServerSDKEvaluationSeriesContext eval_context);

/**
 * @brief Get the hook context provided by the caller.
 *
 * This contains application-specific data passed to the variation call,
 * such as OpenTelemetry span parents.
 *
 * @param eval_context Evaluation context. Must not be NULL.
 * @return Hook context. Valid only during the callback execution.
 *         Must not be freed. Do not call LDHookContext_Free() on this.
 */
LD_EXPORT(LDHookContext)
LDEvaluationSeriesContext_HookContext(
    LDServerSDKEvaluationSeriesContext eval_context);

/**
 * @brief Get the environment ID, if available.
 *
 * The environment ID is only available after SDK initialization completes.
 * Returns NULL if not yet available.
 *
 * @param eval_context Evaluation context. Must not be NULL.
 * @return Environment ID as null-terminated UTF-8 string, or NULL if not
 *         available. Valid only during the callback execution. Must not
 *         be freed.
 */
LD_EXPORT(char const*)
LDEvaluationSeriesContext_EnvironmentId(
    LDServerSDKEvaluationSeriesContext eval_context);

#ifdef __cplusplus
}
#endif
