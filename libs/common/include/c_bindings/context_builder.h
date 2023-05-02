// NOLINTBEGIN modernize-use-using

#pragma once

#include "./context.h"
#include "./export.h"
#include "./value.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

#ifdef __cplusplus
}
#endif

typedef void* LDContextBuilder;

/**
 * Create a new context builder.
 * @return A new context builder instance.
 */
LD_EXPORT(LDContextBuilder) LDContextBuilder_New();

/**
 * Free a context builder.
 *
 * This method only needs to be used when not building
 * the context. If you use LDContextBuilder_Build, then the builder will
 * be consumed, and you do not need to call LDContextBuilder_Free.
 * @param builder The builder to free.
 */
LD_EXPORT(void) LDContextBuilder_Free(LDContextBuilder builder);

/**
 * Construct a context from a context builder.
 *
 * When building a context using LDContextBuilder_Build the builder will be
 * consumed and you do not need to call LDContextBuilder_Free.
 *
 * @param builder The builder to build a context from.
 * @return The built context.
 */
LD_EXPORT(LDContext) LDContextBuilder_Build(LDContextBuilder builder);

/**
 * Add a kind instance to the context builder. The kind will have the specified
 * key.
 *
 * Attributes may be set for the kind by calling the
 * LDContextBuilder_Attributes_* methods with the same "kind".
 *
 * You must first add the kind to the context builder before setting attributes.
 *
 * If you call LDContextBuilder_AddKind a second time, with an already specified
 * kind, but a different key, then the key for that kind will be updated.
 *
 * @param builder The builder to add the kind to.
 * @param kind The kind to add.
 * @param key The key for that kind.
 */
LD_EXPORT(void)
LDContextBuilder_AddKind(LDContextBuilder builder,
                         char const* kind,
                         char const* key);

/**
 * Add or update an attribute in the specified kind.
 *
 * Adding a LDValue to the builder will consume that value.
 * You should not access the value after adding it to the builder, and you
 * do not need to call LDValue_Free on the value.
 *
 * @param kind The kind to add the attribute to.
 * @param attr_key The key of the attribute to add.
 * @param val The value of the attribute to add.
 */
LD_EXPORT(bool)
LDContextBuilder_Attributes_Set(LDContextBuilder builder,
                                char const* kind,
                                char const* attr_key,
                                LDValue val);

/**
 * Add or update a private attribute. Once an attribute has been set as private
 * using LDContextBuilder_Attributes_SetPrivate it will remain private.
 * A subsequent call to LDContextBuilder_Attributes_Set, for the same attribute,
 * will not remove the private status.
 *
 * This method cannot be used to set the key, kind, name, or anonymous
 * property of a context.
 *
 * Adding a LDValue to the builder will consume that value.
 * You should not access the value after adding it to the builder, and you
 * do not need to call LDValue_Free on the value. This method is just a
 * convenience which also adds the attribute to the private attributes list,
 * as if using LDContextBuilder_Attributes_AddPrivateAttribute.
 *
 * @param kind The kind to set the private attribute for.
 * @param attr_key The key of the private attribute.
 * @param val The value of the private attribute.
 */
LD_EXPORT(bool)
LDContextBuilder_Attributes_SetPrivate(LDContextBuilder builder,
                                       char const* kind,
                                       char const* attr_key,
                                       LDValue val);

/**
 * Set the name attribute for the specified kind.
 *
 * You can search for contexts on the Contexts page by name.
 *
 * This method will make a copy of the name string, and the caller remains
 * responsible for the original name string.
 *
 * @param kind The kind to set the name for.
 * @param name The name to set.
 */
LD_EXPORT(bool)
LDContextBuilder_Attributes_SetName(LDContextBuilder builder,
                                    char const* kind,
                                    char const* name);

/**
 * Set the anonymous attribute for the specified kind.
 *
 * If true, the context will _not_ appear on the Contexts page in the
 * LaunchDarkly dashboard.
 *
 * @param kind The kind to set the anonymous attribute for.
 * @param anonymous The value to set the anonymous attribute to.
 */
LD_EXPORT(bool)
LDContextBuilder_Attributes_SetAnonymous(LDContextBuilder builder,
                                         char const* kind,
                                         bool anonymous);

/**
 * Designate a context attribute, or properties within them, as private:
 * that is, their values will not be sent to LaunchDarkly in analytics
 * events.
 *
 * Each parameter can be a simple attribute name, such as "email". Or, if
 * the first character is a slash, the parameter is interpreted as a
 * slash-delimited path to a property within a JSON object, where the first
 * path component is a Context attribute name and each following component
 * is a nested property name: for example, suppose the attribute "address"
 * had the following JSON object value:
 *
 * ```
 * {"street": {"line1": "abc", "line2": "def"}}
 * ```
 *
 * Using ["/address/street/line1"] in this case would cause the "line1"
 * property to be marked as private. This syntax deliberately resembles JSON
 * Pointer, but other JSON Pointer features such as array indexing are not
 * supported for Private.
 *
 * This action only affects analytics events that involve this particular
 * Context. To mark some (or all) Context attributes as private for all
 * contexts, use the overall configuration for the SDK. See [TODO] and
 * [TODO].
 *
 * The attributes "kind" and "key", and the "_meta" attributes cannot be
 * made private.
 *
 * In this example, firstName is marked as private, but lastName is not:
 *
 * ```
 * [TODO]
 * ```
 *
 * This is a metadata property, rather than an attribute that can be
 * addressed in evaluations: that is, a rule clause that references the
 * attribute name "privateAttributes", will not use this value, but would
 * use a "privateAttributes" attribute set on the context.
 *
 * This method will make a copy of the attr_ref string, and the caller remains
 * responsible for the original name string.
 *
 * @param kind The kind to set the attribute as private for.
 * @param attr_ref An attribute reference.
 */
LD_EXPORT(bool)
LDContextBuilder_Attributes_AddPrivateAttribute(LDContextBuilder builder,
                                                char const* kind,
                                                char const* attr_key);

// NOLINTEND modernize-use-using
