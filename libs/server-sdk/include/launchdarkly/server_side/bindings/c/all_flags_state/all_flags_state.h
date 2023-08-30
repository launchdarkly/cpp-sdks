/** @file */
// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/export.h>
#include <launchdarkly/bindings/c/status.h>

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
 * True if the AllFlagsState is valid.  False if there was
 * an error, such as the data store being unavailable. When false, the other
 * accessors will not return data.
 *
 * An invalid AllFlagsState can still be serialized successfully to a JSON
 * string.
 *
 * @param state The AllFlagState to check for validity.
 * @return True if the state is valid, false otherwise.
 */
LD_EXPORT(bool) LDAllFlagsState_Valid(LDAllFlagsState state);

/**
 * Defines options that may be used with LDServerSDK_AllFlagsState. To obtain
 * default behavior, pass LD_ALLFLAGSSTATE_DEFAULT.
 *
 * It is possible to combine multiple options by ORing them together.
 *
 * Example:
 * @code
 * LDAllFlagsState state = LDServerSDK_AllFlagsState(sdk, context,
 *          LD_ALLFLAGSSTATE_INCLUDE_REASONS | LD_ALLFLAGSSTATE_CLIENT_SIDE_ONLY
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
