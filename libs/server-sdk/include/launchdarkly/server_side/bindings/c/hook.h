/**
 * @file hook.h
 * @brief C bindings for LaunchDarkly SDK hooks.
 *
 * Hooks allow you to instrument the SDK's evaluation and tracking behavior
 * for purposes like logging, analytics, or distributed tracing (e.g. OpenTelemetry).
 *
 * LIFETIME AND OWNERSHIP:
 * - The LDServerSDKHook struct should be allocated by the caller (stack or heap)
 * - The function pointers and UserData pointer are copied when the hook is
 *   registered with the config builder
 * - UserData lifetime must extend for the entire lifetime of the SDK client
 * - All context parameters passed to callbacks are temporary - valid only
 *   during the callback execution
 * - EvaluationSeriesData returned from callbacks transfers ownership to the SDK
 */
// NOLINTBEGIN(modernize-use-using)
#pragma once

#include <launchdarkly/bindings/c/export.h>
#include <launchdarkly/bindings/c/data/evaluation_detail.h>

#ifdef __cplusplus
extern "C" { // only need to export C interface if used by C++ source code
#endif

typedef struct p_LDServerSDKEvaluationSeriesContext* LDServerSDKEvaluationSeriesContext;
typedef struct p_LDServerSDKEvaluationSeriesData* LDServerSDKEvaluationSeriesData;
typedef struct p_LDServerSDKTrackSeriesContext* LDServerSDKTrackSeriesContext;

/**
 * @brief Callback invoked before a flag evaluation.
 *
 * Use this to instrument evaluations, such as starting a span for distributed
 * tracing.
 *
 * PARAMETERS:
 * @param series_context Read-only context about the evaluation. Valid only
 *                       during this callback.
 * @param data Mutable data that can be passed to afterEvaluation. Ownership
 *             transfers to the SDK. May be NULL initially.
 * @param user_data Application-specific context pointer set when creating
 *                  the hook.
 *
 * RETURNS:
 * EvaluationSeriesData to pass to afterEvaluation. Return the input data
 * unmodified if you don't need to add anything. Ownership transfers to SDK.
 * If NULL is returned, an empty data object will be created.
 *
 * LIFETIME:
 * - series_context: Valid only during callback execution - do not store
 * - data: Ownership transfers to SDK
 * - user_data: Managed by caller - must remain valid for SDK lifetime
 */
typedef LDServerSDKEvaluationSeriesData (*LDServerSDKHook_BeforeEvaluation)(
    LDServerSDKEvaluationSeriesContext series_context,
    LDServerSDKEvaluationSeriesData data,
    void* user_data);

/**
 * @brief Callback invoked after a flag evaluation.
 *
 * Use this to instrument evaluation results, such as ending a span for
 * distributed tracing or logging the result.
 *
 * PARAMETERS:
 * @param series_context Read-only context about the evaluation. Valid only
 *                       during this callback.
 * @param data Mutable data passed from beforeEvaluation. Ownership transfers
 *             to SDK. May contain data from previous stages.
 * @param detail The evaluation result. Valid only during this callback.
 * @param user_data Application-specific context pointer.
 *
 * RETURNS:
 * EvaluationSeriesData for potential future stages. Return the input data
 * unmodified if you don't need to modify it. Ownership transfers to SDK.
 * If NULL is returned, an empty data object will be created.
 *
 * LIFETIME:
 * - series_context: Valid only during callback execution
 * - data: Ownership transfers to SDK
 * - detail: Valid only during callback execution
 * - user_data: Managed by caller
 */
typedef LDServerSDKEvaluationSeriesData (*LDServerSDKHook_AfterEvaluation)(
    LDServerSDKEvaluationSeriesContext series_context,
    LDServerSDKEvaluationSeriesData data,
    LDEvalDetail detail,
    void* user_data);

/**
 * @brief Callback invoked after a track event.
 *
 * Use this to instrument custom events, such as logging or adding
 * tracing information.
 *
 * PARAMETERS:
 * @param series_context Read-only context about the track call. Valid only
 *                       during this callback.
 * @param user_data Application-specific context pointer.
 *
 * RETURNS: void (no data is passed between track stages)
 *
 * LIFETIME:
 * - series_context: Valid only during callback execution
 * - user_data: Managed by caller
 */
typedef void (*LDServerSDKHook_AfterTrack)(LDServerSDKTrackSeriesContext series_context,
                                           void* user_data);

/**
 * @brief Hook structure containing callback function pointers.
 *
 * USAGE:
 * 1. Allocate an LDServerSDKHook struct (stack or heap)
 * 2. Call LDServerSDKHook_Init() to initialize it
 * 3. Set the Name field (required, UTF-8 encoded, null-terminated)
 * 4. Set any callback function pointers you need (NULL if not used)
 * 5. Set UserData to application-specific context (NULL if not needed)
 * 6. Register with LDServerConfigBuilder_Hooks()
 *
 * EXAMPLE:
 * @code
 *   struct LDServerSDKHook my_hook;
 *   LDServerSDKHook_Init(&my_hook);
 *   my_hook.Name = "MyTracingHook";
 *   my_hook.BeforeEvaluation = my_before_callback;
 *   my_hook.AfterEvaluation = my_after_callback;
 *   my_hook.UserData = my_context;
 *   LDServerConfigBuilder_Hooks(builder, my_hook);
 * @endcode
 *
 * LIFETIME:
 * - The LDServerSDKHook struct itself can be stack-allocated or freed after
 *   registration (the SDK copies it)
 * - The Name string must remain valid until LDServerConfigBuilder_Build()
 * - UserData must remain valid for the entire SDK client lifetime
 */
struct LDServerSDKHook {
    /**
     * Name of the hook. Required. Must be a null-terminated UTF-8 string.
     * Must remain valid until LDServerConfigBuilder_Build() is called.
     */
    char const* Name;

    /**
     * Optional callback invoked before evaluations.
     * Set to NULL if not needed.
     */
    LDServerSDKHook_BeforeEvaluation BeforeEvaluation;

    /**
     * Optional callback invoked after evaluations.
     * Set to NULL if not needed.
     */
    LDServerSDKHook_AfterEvaluation AfterEvaluation;

    /**
     * Optional callback invoked after track calls.
     * Set to NULL if not needed.
     */
    LDServerSDKHook_AfterTrack AfterTrack;

    /**
     * Application-specific context pointer passed to all callbacks.
     * Must remain valid for the entire SDK client lifetime.
     * May be NULL if not needed.
     */
    void* UserData;
};

/**
 * @brief Initialize a hook structure to safe defaults.
 *
 * Sets all function pointers and UserData to NULL, and Name to NULL.
 * Must be called before setting any fields.
 *
 * @param hook Pointer to hook structure to initialize. Must not be NULL.
 */
LD_EXPORT(void)
LDServerSDKHook_Init(struct LDServerSDKHook* hook);

#ifdef __cplusplus
}
#endif

// NOLINTEND(modernize-use-using)
