// NOLINTBEGIN modernize-use-using

#pragma once

#include "./export.h"
#include "./value.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef void* LDContext;
typedef void* LDContext_PrivateAttributesIter;

/**
 * Check if the context is valid.
 *
 * @param context The context to check.
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
 * @param context The context to get an attribute from
 * @param kind The kind within the context to get the attribute from.
 * @param ref An attribute reference representing the attribute to get.
 * @return The attribute value or a null pointer.
 */
LD_EXPORT(LDValue)
LDContext_Get(LDContext context, char const* kind, char const* ref);

/**
 * If the context is not valid, then get a string containing the reason the
 * context is not valid.
 *
 * The lifetime of the returned string is tied to the LDContext.
 *
 * @param context The context to check for validity.
 * @return A string explaining why the context is not valid.
 */
LD_EXPORT(char const*) LDContext_Errors(LDContext context);

LD_EXPORT(LDContext_PrivateAttributesIter)
LDContext_CreatePrivateAttributesIter(LDContext context, const char* kind);

LD_EXPORT(void)
LDContext_DestroyPrivateAttributesIter(LDContext_PrivateAttributesIter iter);

LD_EXPORT(void)
LDContext_PrivateAttributesIter_Next(LDContext_PrivateAttributesIter iter);

LD_EXPORT(bool)
LDContext_PrivateAttributesIter_End(LDContext_PrivateAttributesIter iter);

LD_EXPORT(char const*)
LDContext_PrivateAttributesIter_Value(LDContext_PrivateAttributesIter iter);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
