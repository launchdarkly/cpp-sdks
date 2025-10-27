/**
 * @file evaluation_series_data.h
 * @brief C bindings for hook data passed between evaluation stages.
 *
 * EvaluationSeriesData is mutable data that hooks can use to pass information
 * from beforeEvaluation to afterEvaluation. This is useful for:
 * - Storing timing information
 * - Passing span contexts for distributed tracing
 * - Accumulating custom metrics
 *
 * LIFETIME AND OWNERSHIP:
 * - Data returned from hook callbacks transfers ownership to the SDK
 * - Data received in callbacks can be modified and returned (ownership transfer)
 * - Keys are copied by the SDK
 * - Values retrieved with GetValue() are temporary - valid only during callback
 * - Do not call LDValue_Free() on values retrieved with GetValue()
 * - Values stored with SetValue() are copied into the data object
 * - Pointers (void*) lifetime is managed by the application
 *
 * BUILDER PATTERN:
 * To modify data, use LDEvaluationSeriesData_NewBuilder() to create a builder,
 * make changes, then build a new data object.
 */

#pragma once

#include <launchdarkly/bindings/c/export.h>
#include <launchdarkly/bindings/c/context.h>

// No effect in C++, but we want it for C.
// ReSharper disable once CppUnusedIncludeDirective
#include <stdbool.h> // NOLINT(*-deprecated-headers)
// ReSharper disable once CppUnusedIncludeDirective
#include <stddef.h> // NOLINT(*-deprecated-headers)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct p_LDServerSDKEvaluationSeriesData* LDServerSDKEvaluationSeriesData;
typedef struct p_LDEvaluationSeriesDataBuilder* LDServerSDKEvaluationSeriesDataBuilder;


/**
 * @brief Create a new empty evaluation series data.
 *
 * @return New data object. Must be freed with LDEvaluationSeriesData_Free()
 *         or transferred to SDK via hook callback return.
 */
LD_EXPORT(LDServerSDKEvaluationSeriesData)
LDEvaluationSeriesData_New(void);

/**
 * @brief Get a Value from the evaluation series data.
 *
 * LIFETIME: Returns a temporary value valid only during the callback.
 * Do not call LDValue_Free() on the returned value.
 *
 * USAGE:
 * @code
 *   LDValue value;
 *   if (LDEvaluationSeriesData_GetValue(data, "timestamp", &value)) {
 *       // Use value (valid only during callback)
 *       double timestamp = LDValue_GetNumber(value);
 *       // Do NOT call LDValue_Free(value)
 *   }
 * @endcode
 *
 * @param data Data object. Must not be NULL.
 * @param key Key to look up. Must be null-terminated UTF-8 string.
 *            Must not be NULL.
 * @param out_value Pointer to receive the value. Must not be NULL.
 *                  Set to a temporary LDValue (valid only during callback).
 *                  Do not call LDValue_Free() on this value.
 * @return true if key was found and contains a Value, false otherwise.
 */
LD_EXPORT(bool)
LDEvaluationSeriesData_GetValue(LDServerSDKEvaluationSeriesData data,
                                char const* key,
                                LDValue* out_value);

/**
 * @brief Get a pointer from the evaluation series data.
 *
 * Retrieves a pointer previously stored with
 * LDEvaluationSeriesDataBuilder_SetPointer().
 *
 * USAGE - OpenTelemetry span:
 * @code
 *   void* span;
 *   if (LDEvaluationSeriesData_GetPointer(data, "span", &span)) {
 *       // Cast and use span
 *       MySpan* typed_span = (MySpan*)span;
 *   }
 * @endcode
 *
 * @param data Data object. Must not be NULL.
 * @param key Key to look up. Must be null-terminated UTF-8 string.
 *            Must not be NULL.
 * @param out_pointer Pointer to receive the pointer value. Must not be NULL.
 *                    Set to NULL if key not found.
 * @return true if key was found and contains a pointer, false otherwise.
 */
LD_EXPORT(bool)
LDEvaluationSeriesData_GetPointer(LDServerSDKEvaluationSeriesData data,
                                  char const* key,
                                  void** out_pointer);

/**
 * @brief Create a builder from existing data.
 *
 * Creates a builder initialized with the contents of the data object.
 * Use this to add or modify entries in the data.
 *
 * USAGE:
 * @code
 *   LDServerSDKEvaluationSeriesDataBuilder builder =
 *       LDEvaluationSeriesData_NewBuilder(input_data);
 *   LDEvaluationSeriesDataBuilder_SetValue(builder, "key", value);
 *   return LDEvaluationSeriesDataBuilder_Build(builder);
 * @endcode
 *
 * @param data Data to copy into builder. May be NULL (creates empty builder).
 * @return Builder object. Must be freed with
 *         LDEvaluationSeriesDataBuilder_Free() or consumed with
 *         LDEvaluationSeriesDataBuilder_Build().
 */
LD_EXPORT(LDServerSDKEvaluationSeriesDataBuilder)
LDEvaluationSeriesData_NewBuilder(LDServerSDKEvaluationSeriesData data);

/**
 * @brief Free evaluation series data.
 *
 * Only call this if you created the data and are not returning it from
 * a hook callback. Data returned from callbacks is owned by the SDK.
 *
 * @param data Data to free. May be NULL (no-op).
 */
LD_EXPORT(void)
LDEvaluationSeriesData_Free(LDServerSDKEvaluationSeriesData data);

/**
 * @brief Set a Value in the builder.
 *
 * OWNERSHIP: The value is copied/moved into the builder. You are responsible
 * for freeing the original value if needed.
 *
 * @param builder Builder object. Must not be NULL.
 * @param key Key for the value. Must be null-terminated UTF-8 string.
 *            Must not be NULL. The key is copied.
 * @param value Value to store. Must not be NULL. The value is copied/moved.
 */
LD_EXPORT(void)
LDEvaluationSeriesDataBuilder_SetValue(LDServerSDKEvaluationSeriesDataBuilder builder,
                                       char const* key,
                                       LDValue value);

/**
 * @brief Set a pointer in the builder.
 *
 * Stores an application-specific pointer. Useful for storing objects like
 * OpenTelemetry spans that need to be passed from beforeEvaluation to
 * afterEvaluation.
 *
 * LIFETIME: The pointer lifetime must extend through the evaluation series
 * (from beforeEvaluation through afterEvaluation).
 *
 * EXAMPLE - OpenTelemetry span:
 * @code
 *   MySpan* span = start_span();
 *   LDEvaluationSeriesDataBuilder_SetPointer(builder, "span", span);
 * @endcode
 *
 * @param builder Builder object. Must not be NULL.
 * @param key Key for the pointer. Must be null-terminated UTF-8 string.
 *            Must not be NULL. The key is copied.
 * @param pointer Pointer to store. May be NULL. Lifetime managed by caller.
 */
LD_EXPORT(void)
LDEvaluationSeriesDataBuilder_SetPointer(
    LDServerSDKEvaluationSeriesDataBuilder builder,
    char const* key,
    void* pointer);

/**
 * @brief Build the evaluation series data.
 *
 * Consumes the builder and creates a data object. After calling this,
 * do not call LDEvaluationSeriesDataBuilder_Free() on the builder.
 *
 * @param builder Builder to consume. Must not be NULL.
 * @return Data object. Must be freed with LDEvaluationSeriesData_Free()
 *         or transferred to SDK via hook callback return.
 */
LD_EXPORT(LDServerSDKEvaluationSeriesData)
LDEvaluationSeriesDataBuilder_Build(LDServerSDKEvaluationSeriesDataBuilder builder);

/**
 * @brief Free a builder without building.
 *
 * Only call this if you did not call LDEvaluationSeriesDataBuilder_Build().
 *
 * @param builder Builder to free. May be NULL (no-op).
 */
LD_EXPORT(void)
LDEvaluationSeriesDataBuilder_Free(LDServerSDKEvaluationSeriesDataBuilder builder);

#ifdef __cplusplus
}
#endif
