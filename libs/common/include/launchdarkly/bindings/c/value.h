/** @file */
// NOLINTBEGIN modernize-use-using

#pragma once

#include <stdbool.h>

#include <launchdarkly/bindings/c/export.h>

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
 * An array-type or object-type LDValue must be created using @ref
 * LDArrayBuilder_New() or LDObjectBuilder_New().
 *
 * Basic LDValue types can be converted to raw types using the LDValue_Get*
 * methods.
 *
 * Accessing the members of object-type or array-type type must be done using
 * iteration.
 */
typedef struct _LDValue* LDValue;

/**
 * LDValue_ObjectIter is a handle to an iterator, bound to an @ref LDValue.
 * It can be used to obtain the keys and values of an object.
 *
 * The iterator must be destroyed after use using @ref LDValue_ObjectIter_Free.
 * An iterator for an LDValue that has been freed should not be used.
 */
typedef struct _LDValue_ObjectIter* LDValue_ObjectIter;

/**
 * LDValue_ArrayIter is a handle to an iterator, bound to an @ref LDValue.
 * It can be used to obtain the values of an array.
 *
 * The iterator must be destroyed after use using @ref LDValue_ArrayIter_Free.
 * An iterator for an LDValue that has been freed should not be used.
 */
typedef struct _LDValue_ArrayIter* LDValue_ArrayIter;

/**
 * Allocates a new null-type @ref LDValue.
 * *WARNING!* A `NULL` pointer is not a valid LDValue; to represent null (the
 * JSON type), use this constructor.
 *
 * @return New LDValue.
 */
LD_EXPORT(LDValue)
LDValue_NewNull();

/**
 * Allocates a new boolean-type @ref LDValue.
 * @param val Boolean.
 * @return New LDValue.
 */
LD_EXPORT(LDValue)
LDValue_NewBool(bool val);

/**
 * Allocates a new number-type @ref LDValue.
 * @param val Double value.
 * @return New LDValue.
 */
LD_EXPORT(LDValue)
LDValue_NewNumber(double val);

/**
 * Allocates a new string-type @ref LDValue.
 *
 * The input string will be copied.
 *
 * @param val Constant reference to a string. The string is copied. Must not be
 * NULL.
 * @return New LDValue.
 */
LD_EXPORT(LDValue)
LDValue_NewString(char const* val);

/**
 * Allocates an @ref LDValue by cloning an existing LDValue.
 *
 * @param val The source value. Must not be NULL.
 * @return New LDValue.
 */
LD_EXPORT(LDValue)
LDValue_NewValue(LDValue val);

/**
 * Frees an @ref LDValue.
 *
 * An LDValue should only be freed when directly owned by the caller, i.e.,
 * it was never moved into an array or object builder.
 *
 * @param val LDValue to free.
 */
LD_EXPORT(void)
LDValue_Free(LDValue val);

/**
 * Returns the type of an @ref LDValue.
 * @param val LDValue to inspect. Must not be NULL.
 * @return Type of the LDValue, or @ref LDValueType_Unrecognized if the type is
 * unrecognized.
 */
LD_EXPORT(enum LDValueType)
LDValue_Type(LDValue val);

/**
 * Obtain value of a boolean-type @ref LDValue, otherwise returns false.
 *
 * @param val Target LDValue. Must not be NULL.
 * @return Boolean value, or false if not boolean-type.
 */
LD_EXPORT(bool)
LDValue_GetBool(LDValue val);

/**
 * Obtain value of a number-type @ref LDValue, otherwise return 0.
 * @param val Target LDValue. Must not be NULL.
 * @return Number value, or 0 if not number-type.
 */
LD_EXPORT(double)
LDValue_GetNumber(LDValue val);

/**
 * Obtain value of a string-type @ref LDValue, otherwise returns pointer
 * to an empty string.
 *
 * The returned string is only valid for the lifetime of
 * the LDValue. If you need the string outside this lifetime, then a copy
 * should be made.
 *
 * @param val Target LDValue. Must not be NULL.
 * @return String value, or empty string if not string-type.
 */
LD_EXPORT(char const*)
LDValue_GetString(LDValue val);

/**
 * Obtain number of @ref LDValue elements stored in an array-type LDValue, or
 * number of key/LDValue pairs stored in an object-type LDValue.
 *
 * If not an array-type or object-type, returns 0.
 *
 * @param val Target LDValue. Must not be NULL.
 * @return Count of LDValue elements, or 0 if not array-type/object-type.
 */
LD_EXPORT(unsigned int)
LDValue_Count(LDValue val);

/**
 * Serializes the LDValue to a JSON value. The returning string should be
 * freed with @ref LDMemory_FreeString.
 *
 * Please note that numbers are serialized using scientific notation;
 * for example the number 17 would be serialized as '1.7E1'.
 *
 * @param val Target LDValue. Must not be NULL.
 * @return A string containing the JSON representation of the LDValue. The
 * string should be freed with @ref LDMemory_FreeString.
 *
 */
LD_EXPORT(char*)
LDValue_SerializeJSON(LDValue val);

/**
 * Obtain iterator over an array-type @ref LDValue, otherwise NULL.
 *
 * The iterator starts at the first element.
 *
 * @param val Target LDValue. Must not be NULL.
 * @return Iterator, or NULL if not an array-type. The iterator
 * must should be destroyed with LDValue_ArrayIter_Free().
 */
LD_EXPORT(LDValue_ArrayIter)
LDValue_ArrayIter_New(LDValue val);

/**
 * Move the array-type iterator to the next item. Should only be done for an
 * iterator which is not at the end.
 *
 * @param iter The iterator to advance. Must not be NULL.
 */
LD_EXPORT(void)
LDValue_ArrayIter_Next(LDValue_ArrayIter iter);

/**
 * Check if an array-type iterator is at the end.
 *
 * @param iter The iterator to check. Must not be NULL.
 * @return True if the iterator is at the end.
 */
LD_EXPORT(bool)
LDValue_ArrayIter_End(LDValue_ArrayIter iter);

/**
 * Get the value for the array-type iterator. The value's lifetime is valid
 * only for as long as the iterator. To obtain a copy, call @ref
 * LDValue_NewValue with the result.
 *
 * @param iter The iterator to get a value for. Must not be NULL.
 * @return The value reference.
 */
LD_EXPORT(LDValue)
LDValue_ArrayIter_Value(LDValue_ArrayIter iter);

/**
 * Destroy an array iterator.
 * @param iter The iterator to destroy.
 */
LD_EXPORT(void)
LDValue_ArrayIter_Free(LDValue_ArrayIter iter);

/**
 * Obtain iterator over an object-type @ref LDValue, otherwise NULL.
 *
 * The iterator starts at the first element.
 *
 * @param val Target LDValue. Must not be NULL.
 * @return Iterator, or NULL if not an object-type. The iterator
 * must should be destroyed with LDValue_ObjectIter_Free().
 */
LD_EXPORT(LDValue_ObjectIter)
LDValue_ObjectIter_New(LDValue val);

/**
 * Move the object-type iterator to the next item. Should only be done for an
 * iterator which is not at the end.
 *
 * @param iter The iterator to advance. Must not be NULL.
 */
LD_EXPORT(void)
LDValue_ObjectIter_Next(LDValue_ObjectIter iter);

/**
 * Check if an object-type iterator is at the end.
 *
 * @param iter The iterator to check. Must not be NULL.
 * @return True if the iterator is at the end.
 */
LD_EXPORT(bool)
LDValue_ObjectIter_End(LDValue_ObjectIter iter);

/**
 * Get the value for an object-type iterator. The value's lifetime is valid
 * only for as long as the iterator. To obtain a copy, call @ref
 * LDValue_NewValue.
 *
 * @param iter The iterator to get a value for. Must not be NULL.
 * @return The value.
 */
LD_EXPORT(LDValue)
LDValue_ObjectIter_Value(LDValue_ObjectIter iter);

/**
 * Get the key for an object-type iterator.
 *
 * The returned key has a lifetime attached to that of the @ref LDValue.
 *
 * @param iter The iterator to get a key for. Must not be NULL.
 * @return The key.
 */
LD_EXPORT(char const*)
LDValue_ObjectIter_Key(LDValue_ObjectIter iter);

/**
 * Destroy an object iterator.
 * @param iter The iterator to destroy.
 */
LD_EXPORT(void)
LDValue_ObjectIter_Free(LDValue_ObjectIter iter);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
