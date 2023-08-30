/** @file */
// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/export.h>
#include <launchdarkly/bindings/c/status.h>
#include <launchdarkly/bindings/c/value.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDAllFlagsState* LDAllFlagsState;

/**
 * Frees an AllFlagsState.
 * @param state The AllFlagState to free.
 */
LD_EXPORT(void) LDAllFlagsState_Free(LDAllFlagsState state);

/**
 * True if the LDAllFlagsState is valid.  False if there was
 * an error, such as the data store being unavailable. When false, the other
 * accessors will not return data.
 *
 * An invalid LDAllFlagsState can still be serialized successfully to a JSON
 * string.
 *
 * @param state The LDAllFlagState to check for validity. Must not be NULL.
 * @return True if the state is valid, false otherwise.
 */
LD_EXPORT(bool) LDAllFlagsState_Valid(LDAllFlagsState state);

/**
 * Serializes the LDAllFlagsState to a JSON string.
 *
 * This JSON is suitable for bootstrapping a client-side SDK.
 *
 * @param state The LDAllFlagState to serialize. Must not be NULL.
 * @return A JSON string representing the LDAllFlagsState. The caller must free
 * the string using LDMemory_FreeString.
 */
LD_EXPORT(char*)
LDAllFlagsState_SerializeJSON(LDAllFlagsState state);

/**
 * Returns the flag value for the context used to generate this
 * LDAllFlagsState.
 *
 * In order to avoid copying large amounts of data when a value is accessed,
 * the returned LDValue is a reference and NOT DIRECTLY OWNED by the caller. Its
 * lifetime is managed by the parent LDAllFlagsState object.
 *
 * WARNING!
 * Do not free the returned LDValue.
 * Do not access in any way the returned LDValue after the LDAllFlagsState has
 * been freed.
 *
 * If the flag has no value, returns an LDValue of type LDValueType_Null.
 *
 * To obtain an owned copy not subject to these restrictions, call
 * LDValue_NewValue on the result.
 *
 * @param state An LDAllFlagsState. Must not be NULL.
 * @param flag_key Key of the flag. Must not be NULL.
 * @return The evaluation result of the flag. The caller MUST NOT free this
 * value and MUST NOT access this value after the LDAllFlagsState has been
 * freed.
 */
LD_EXPORT(LDValue)
LDAllFlagsState_Value(LDAllFlagsState state, char const* flag_key);

/**
 * Defines options that may be used with LDServerSDK_AllFlagsState. To
 * obtain default behavior, pass LD_ALLFLAGSSTATE_DEFAULT.
 *
 * It is possible to combine multiple options by ORing them together.
 *
 * Example:
 * @code
 * LDAllFlagsState state = LDServerSDK_AllFlagsState(sdk, context,
 *          LD_ALLFLAGSSTATE_INCLUDE_REASONS |
 * LD_ALLFLAGSSTATE_CLIENT_SIDE_ONLY
 * );
 * @endcode
 */
enum LDAllFlagsState_Options {
    /**
     * Default behavior.
     */
    LD_ALLFLAGSSTATE_DEFAULT = 0,
    /**
     * Include evaluation reasons in the state object. By default, they
     * are not.
     */
    LD_ALLFLAGSSTATE_INCLUDE_REASONS = (1 << 0),
    /**
     * Include detailed flag metadata only for flags with event tracking
     * or debugging turned on.
     *
     * This reduces the size of the JSON data if you are
     * passing the flag state to the front end.
     */
    LD_ALLFLAGSSTATE_DETAILS_ONLY_FOR_TRACKED_FLAGS = (1 << 1),
    /**
     * Include only flags marked for use with the client-side SDK.
     * By default, all flags are included.
     */
    LD_ALLFLAGSSTATE_CLIENT_SIDE_ONLY = (1 << 2)
};

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
