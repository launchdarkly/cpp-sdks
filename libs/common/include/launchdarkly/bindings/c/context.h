/** @file */
// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/export.h>
#include <launchdarkly/bindings/c/value.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDContext* LDContext;
typedef struct _LDContext_PrivateAttributesIter*
    LDContext_PrivateAttributesIter;

/**
 * Get the canonical key for the context.
 * @param context The context. Must not be NULL.
 * @return Canonical key. Only valid for the lifetime of this context.
 */
LD_EXPORT(char const*)
LDContext_CanonicalKey(LDContext context);

/**
 * Check if the context is valid.
 *
 * @param context The context to check. Must not be NULL.
 * @return True if the context is valid.
 */
LD_EXPORT(bool) LDContext_Valid(LDContext context);

/**
 * Free the context.
 *
 * @param context The context to free.
 */
LD_EXPORT(void) LDContext_Free(LDContext context);

/**
 * Get an attribute value by kind and attribute reference. If the kind is
 * not present, or the attribute not present in the kind, then
 * a null pointer will be returned.
 *
 * The lifetime of the LDValue is tied to the LDContext it was accessed from.
 * Do not access a returned LDValue from a context after the context has been
 * freed.
 *
 * @param context The context to get an attribute from. Must not be NULL.
 * @param kind The kind within the context to get the attribute from. Must not
 * be NULL.
 * @param ref An attribute reference representing the attribute to get. Must not
 * be NULL.
 * @return The attribute value or a NULL pointer.
 */
LD_EXPORT(LDValue)
LDContext_Get(LDContext context, char const* kind, char const* ref);

/**
 * If the context is not valid, then get a string containing the reason the
 * context is not valid.
 *
 * The lifetime of the returned string is tied to the LDContext.
 *
 * @param context The context to check for validity. Must not be NULL.
 * @return A string explaining why the context is not valid.
 */
LD_EXPORT(char const*) LDContext_Errors(LDContext context);

/**
 * Create an iterator which iterates over the private attributes for the
 * context kind. If there is no such context kind, then a null pointer will
 * be returned.
 *
 * The iterator must be destroyed with LDContext_PrivateAttributesIter_Free.
 *
 * An iterator must not be used after the associated Context has been
 * freed.
 *
 * @param context The context containing the kind. Must not be NULL.
 * @param kind The kind to iterate private attributes for. Must not be NULL.
 * @return A private attributes iterator.
 */
LD_EXPORT(LDContext_PrivateAttributesIter)
LDContext_PrivateAttributesIter_New(LDContext context, char const* kind);

/**
 * Destroy the iterator.
 *
 * @param iter The iterator to destroy.
 */
LD_EXPORT(void)
LDContext_PrivateAttributesIter_Free(LDContext_PrivateAttributesIter iter);

/**
 * Move the iterator to the next item.
 *
 * @param iter The iterator to increment. Must not be NULL.
 */
LD_EXPORT(void)
LDContext_PrivateAttributesIter_Next(LDContext_PrivateAttributesIter iter);

/**
 * Check if the iterator is at the end.
 *
 * @param iter The iterator to check. Must not be NULL.
 * @return True if the iterator is at the end.
 */
LD_EXPORT(bool)
LDContext_PrivateAttributesIter_End(LDContext_PrivateAttributesIter iter);

/**
 * Get the value pointed to by the iterator.
 *
 * The lifetime of the returned value is the same as the Context.
 *
 * @param iter The iterator to get a value for. Must not be NULL.
 * @return The attribute reference as a string.
 */
LD_EXPORT(char const*)
LDContext_PrivateAttributesIter_Value(LDContext_PrivateAttributesIter iter);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
