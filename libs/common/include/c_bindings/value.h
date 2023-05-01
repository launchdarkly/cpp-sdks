#pragma once

#include <stdbool.h>

#include "./export.h"

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
              // used by C++ source code
#endif

/**
 * Describes the type of an LDValue.
 * These correspond to the standard types in JSON.
 */
enum LDValueType {
    /**
     * The value is null.
     */
    LDValueType_Null,
    /**
     * The value is a boolean.
     */
    LDValueType_Bool,
    /**
     * The value is a number. JSON does not have separate types for integers and
     * floats.
     */
    LDValueType_Number,
    /**
     * The value is a string.
     */
    LDValueType_String,
    /**
     * The value is an array.
     */
    LDValueType_Array,
    /**
     * The value is an object.
     */
    LDValueType_Object
};

/**
 * Value represents any of the data types supported by JSON, all of which can be
 * used for a LaunchDarkly feature flag variation, or for an attribute in an
 * evaluation context. Value instances are immutable.
 *
 * A basic LDValue types can be created directly using the LDValue_New* methods.
 * This includes: null-type, boolean-type, number-type, and string-type.
 *
 * An array-type or object-type LDValue must be created using LDArrayBuilder or
 * LDObjectBuilder.
 *
 * Basic LDValue types can be converted to raw types using the LDValue_Get*
 * methods.
 *
 * Accessing the members of object-type or array-type type must be done using
 * iteration.
 */
typedef void* LDValue;

/**
 * LDValue_ObjectIter is a handle to an iterator, bound to an LDValue.
 * It can be used to obtain the keys and values of an LDObject.
 *
 * The iterator must be destroyed after use. An iterator for an LDValue
 * that has been freed should not be used.
 */
typedef void* LDValue_ObjectIter;

/**
 * LDValue_ArrayIter is a handle to an iterator, bound to an LDValue.
 * It can be used to obtain the values of an LDArray.
 *
 * The iterator must be destroyed after use. An iterator for an LDValue
 * that has been freed should not be used.
 */
typedef void* LDValue_ArrayIter;

/**
 * Allocates a new null-type LDValue.
 * Note that a NULL pointer is not a valid LDValue; to represent null (the JSON
 * type), use this constructor.
 *
 * @return New LDValue.
 */
LD_EXPORT(LDValue) LDValue_NewNull();

/**
 * Allocates a new boolean-type LDValue.
 * @param val LDBooleanTrue or LDBooleanFalse.
 * @return New LDValue.
 */
LD_EXPORT(LDValue) LDValue_NewBool(bool val);

/**
 * Allocates a new number-type LDValue.
 * @param val Double value.
 * @return New LDValue.
 */
LD_EXPORT(LDValue) LDValue_NewNumber(double val);

/**
 * Allocates a new string-type LDValue.
 *
 * The input string will be copied. To avoid the copy, see
 * LDValue_ConstantString.
 *
 * @param val Constant reference to a string. The string is copied. Cannot be
 * NULL.
 * @return New LDValue.
 */
LD_EXPORT(LDValue) LDValue_NewString(char const* val);

/**
 * Allocates an LDValue by cloning an existing LDValue.
 *
 * @param source Source LDValue. Must not be `NULL`.
 * @return New LDValue.
 */
LD_EXPORT(LDValue) LDValue_NewValue(LDValue val);

/**
 * Frees an LDValue.
 *
 * An LDValue should only be freed when directly owned by the caller, i.e.,
 * it was never moved into an LDArray or LDObject.
 *
 * @param value LDValue to free. No-op if NULL.
 */
LD_EXPORT(void) LDValue_Free(LDValue val);

/**
 * Returns the type of an LDValue.
 * @param value LDValue to inspect. Cannot be NULL.
 * @return Type of the LDValue, or LDValueType_Unrecognized if the type is
 * unrecognized.
 */
LD_EXPORT(enum LDValueType) LDValue_Type(LDValue val);

/**
 * Obtain value of a boolean-type LDValue, otherwise returns LDBooleanFalse.
 *
 * @param value Target LDValue. Cannot be NULL.
 * @return Boolean value, or false if not boolean-type.
 */
LD_EXPORT(bool) LDValue_GetBool(LDValue val);

/**
 * Obtain value of a number-type LDValue, otherwise return 0.
 * @param value Target LDValue. Cannot be NULL.
 * @return Number value, or 0 if not number-type.
 */
LD_EXPORT(double) LDValue_GetNumber(LDValue val);

/**
 * Obtain value of a string-type LDValue, otherwise returns pointer
 * to an empty string. The returned string is only valid for the lifetime of
 * the LDValue. If you need the string outside this lifetime, then a copy
 * should be made.
 *
 * @param value Target LDValue. Cannot be NULL.
 * @return String value, or empty string if not string-type.
 */
LD_EXPORT(char const*) LDValue_GetString(LDValue val);

/**
 * Obtain number of LDValue elements stored in an array-type LDValue, or number
 * of key/LDValue pairs stored in an object-type LDValue.
 *
 * If not an array-type or object-type, returns 0.
 *
 * @param value Target LDValue. Cannot be NULL.
 * @return Count of LDValue elements, or 0 if not array-type/object-type.
 */
LD_EXPORT(unsigned int) LDValue_Count(LDValue val);

/**
 * Obtain iterator over an array-type LDValue, otherwise NULL.
 *
 * The iterator starts at the first element.
 *
 * @param value Target LDValue. Cannot be NULL.
 * @return Iterator, or NULL if not an array-type. The iterator
 * must should be destroyed with LDValue_DestroyArrayIter.
 */
LD_EXPORT(LDValue_ArrayIter) LDValue_CreateArrayIter(LDValue val);

/**
 * Move the array-type iterator to the next item. Should only be done for an
 * iterator which is not at the end.
 *
 * @param iter The iterator to advance.
 */
LD_EXPORT(void) LDValue_ArrayIter_Next(LDValue_ArrayIter iter);

/**
 * Check if an array-type iterator is at the end.
 *
 * @param iter The iterator to check.
 * @return True if the iterator is at the end.
 */
LD_EXPORT(bool) LDValue_ArrayIter_End(LDValue_ArrayIter iter);

/**
 * Get the value for the array-type iterator.
 *
 * @param iter The iterator to get a value for.
 * @return The value.
 */
LD_EXPORT(LDValue) LdValue_ArrayIter_Value(LDValue_ArrayIter iter);

/**
 * Destroy an array iterator.
 * @param iter The iterator to destroy.
 */
LD_EXPORT(void) LDValue_DestroyArrayIter(LDValue_ArrayIter iter);

/**
 * Obtain iterator over an object-type LDValue, otherwise NULL.
 *
 * The iterator starts at the first element.
 *
 * @param value Target LDValue. Cannot be NULL.
 * @return Iterator, or NULL if not an object-type. The iterator
 * must should be destroyed with LDValue_DestroyObjectIter.
 */
LD_EXPORT(LDValue_ObjectIter) LDValue_CreateObjectIter(LDValue val);

/**
 * Move the object-type iterator to the next item. Should only be done for an
 * iterator which is not at the end.
 *
 * @param iter The iterator to advance.
 */
LD_EXPORT(void) LDValue_ObjectIter_Next(LDValue_ObjectIter iter);

/**
 * Check if an object-type iterator is at the end.
 *
 * @param iter The iterator to check.
 * @return True if the iterator is at the end.
 */
LD_EXPORT(bool) LDValue_ObjectIter_End(LDValue_ObjectIter iter);

/**
 * Get the value for an object-type iterator.
 *
 * @param iter The iterator to get a value for.
 * @return The value.
 */
LD_EXPORT(LDValue) LdValue_ObjectIter_Value(LDValue_ObjectIter iter);

/**
 * Get the key for an object-type iterator.
 *
 * The returned key has a lifetime attached to that of the LDValue.
 *
 * @param iter The iterator to get a key for.
 * @return The key.
 */
LD_EXPORT(char const*) LdValue_ObjectIter_Key(LDValue_ObjectIter iter);

/**
 * Destroy an object iterator.
 * @param iter The iterator to destroy.
 */
LD_EXPORT(void) LDValue_DestroyObjectIter(LDValue_ObjectIter iter);

#ifdef __cplusplus
}
#endif