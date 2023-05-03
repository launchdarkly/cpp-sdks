// NOLINTBEGIN modernize-use-using

#pragma once

#include "./export.h"
#include "./value.h"

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDArrayBuilder* LDArrayBuilder;

/**
 * Construct a new array builder.
 * @return The new array builder.
 *
 */
LD_EXPORT(LDArrayBuilder) LDArrayBuilder_New();

/**
 * Free an array builder. This should only be done for a builder which
 * has not been built. Calling LDArrayBuilder_Build on an array builder
 * transfers consumes the array builder.
 *
 * @param array_builder The builder to free.
 */
LD_EXPORT(void) LDArrayBuilder_Free(LDArrayBuilder array_builder);

/**
 * Add a value to an array builder.
 *
 * After calling this method the provider LDValue is consumed. It should not
 * be accessed, and the caller doesn't need to call LDValue_Free.
 *
 * @param array_builder The array builder to add the value to.
 * @param val The value to add.
 */
LD_EXPORT(void) LDArrayBuilder_Add(LDArrayBuilder array_builder, LDValue val);

/**
 * Construct an LDValue from an array builder.
 *
 * After calling this method the array builder is consumed. It should not be
 * used and the caller does not need to call LDArrayBuilder_Free.
 *
 * @param array_builder The array builder to build an LDValue from.
 * @return The built LDValue.
 */
LD_EXPORT(LDValue) LDArrayBuilder_Build(LDArrayBuilder array_builder);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
