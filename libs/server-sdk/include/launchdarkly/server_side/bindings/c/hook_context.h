/**
 * @file hook_context.h
 * @brief C bindings for passing caller data to hooks.
 *
 * HookContext allows application code to pass arbitrary data through to hooks.
 * This is useful for propagating context like OpenTelemetry span parents in
 * asynchronous web frameworks where thread-local storage doesn't work.
 *
 * USAGE:
 * Most applications don't need HookContext. Only use it when you need to pass
 * data from the evaluation call site to your hooks, such as:
 * - OpenTelemetry span parents in async frameworks
 * - Request IDs for distributed tracing
 * - Custom metadata for logging
 *
 * LIFETIME AND OWNERSHIP:
 * - Hook context must remain valid for the duration of the variation call
 * - Data stored in hook context must be managed by the caller
 * - Hook context can be stack-allocated and freed after the variation call
 */

#pragma once

#include <launchdarkly/bindings/c/export.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque hook context handle.
 *
 * Created by LDHookContext_New(), must be freed with LDHookContext_Free().
 */
typedef struct p_LDHookContext* LDHookContext;

/**
 * @brief Create a new hook context.
 *
 * @return New hook context. Must be freed with LDHookContext_Free().
 */
LD_EXPORT(LDHookContext)
LDHookContext_New(void);

/**
 * @brief Set a pointer value in the hook context.
 *
 * Stores an application-specific pointer that can be retrieved by hooks.
 * The lifetime of the pointed-to data must extend through the variation call.
 *
 * EXAMPLE - OpenTelemetry span parent:
 * @code
 *   LDHookContext ctx = LDHookContext_New();
 *   LDHookContext_Set(ctx, "span_parent", my_span_context);
 *   bool result = LDClientSDK_BoolVariation_WithHookContext(
 *       client, context, "flag-key", false, ctx);
 *   LDHookContext_Free(ctx);
 * @endcode
 *
 * @param hook_context Hook context. Must not be NULL.
 * @param key Key for the value. Must be null-terminated UTF-8 string.
 *            Must not be NULL.
 * @param value Pointer to application data. May be NULL.
 *              Lifetime managed by caller - must remain valid through
 *              the variation call.
 */
LD_EXPORT(void)
LDHookContext_Set(LDHookContext hook_context,
                  char const* key,
                  void const* value);

/**
 * @brief Get a pointer value from the hook context.
 *
 * Retrieves an application-specific pointer previously stored with
 * LDHookContext_Set().
 *
 * USAGE IN HOOKS:
 * @code
 *   void* span_parent;
 *   if (LDHookContext_Get(hook_ctx, "span_parent", &span_parent)) {
 *       // Use span_parent
 *   }
 * @endcode
 *
 * @param hook_context Hook context. Must not be NULL.
 * @param key Key to look up. Must be null-terminated UTF-8 string.
 *            Must not be NULL.
 * @param out_value Pointer to receive the value. Must not be NULL.
 *                  Set to NULL if key not found.
 * @return true if key was found, false otherwise.
 */
LD_EXPORT(bool)
LDHookContext_Get(LDHookContext hook_context,
                  char const* key,
                  void const** out_value);

/**
 * @brief Free a hook context.
 *
 * @param hook_context Hook context to free. May be NULL (no-op).
 */
LD_EXPORT(void)
LDHookContext_Free(LDHookContext hook_context);

#ifdef __cplusplus
}
#endif
