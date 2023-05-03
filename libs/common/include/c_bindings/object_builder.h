// NOLINTBEGIN modernize-use-using

#pragma once

#include "./export.h"
#include "./value.h"

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDObjectBuilder* LDObjectBuilder;

/**
 * Construct a new object builder.
 * @return The new object builder.
 *
 */
LD_EXPORT(LDObjectBuilder) LDObjectBuilder_New();

/**
 * Free an object builder. This should only be done for a builder which
 * has not been built. Calling LDArrayBuilder_Build on an array builder
 * transfers consumes the array builder.
 *
 * @param builder The builder to free.
 */
LD_EXPORT(void) LDObjectBuilder_Free(LDObjectBuilder builder);

/**
 * Add a key-value pair to the object builder.
 *
 * After calling this method the provider LDValue is consumed. It should not
 * be accessed, and the caller doesn't need to call LDValue_Free. The key will
 * be copied.
 *
 * @param array_builder The object builder to add the value to.
 * @param key The key for the value being added.
 * @param val The value to add.
 */
LD_EXPORT(void)
LDObjectBuilder_Add(LDObjectBuilder builder, char const* key, LDValue val);

/**
 * Construct an LDValue from an object builder.
 *
 * After calling this method the object builder is consumed. It should not be
 * used and the caller does not need to call LDObjectBuilder_Free.
 *
 * @param builder The object builder to build an LDValue from.
 * @return The built LDValue.
 */
LD_EXPORT(LDValue) LDObjectBuilder_Build(LDObjectBuilder builder);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
