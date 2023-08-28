/** @file */
// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/export.h>
#include <launchdarkly/bindings/c/value.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef void (*FlagChangedCallbackFn)(char const* flag_key,
                                      LDValue new_value,
                                      LDValue old_value,
                                      bool deleted,
                                      void* user_data);

/**
 * Defines a feature flag listener which may be used to listen for flag changes.
 * The struct should be initialized using LDFlagListener_Init before use.
 */
struct LDFlagListener {
    /**
     * Callback function which is invoked for flag changes.
     *
     * The provided pointers are only valid for the duration of the function
     * call (excluding UserData, whose lifetime is controlled by the caller).
     *
     * @param flag_key The name of the flag that changed.
     * @param new_value The new value of the flag. If there was not an new
     * value, because the flag was deleted, then the LDValue will be of a null
     * type. Check the deleted parameter to see if a flag was deleted.
     * @param old_value The old value of the flag. If there was not an old
     * value, for instance a newly created flag, then the Value will be of a
     * null type.
     * @param deleted True if the flag has been deleted.
     */
    FlagChangedCallbackFn FlagChanged;

    /**
     * UserData is forwarded into callback functions.
     */
    void* UserData;
};

/**
 * Initializes a flag listener. Must be called before passing the listener
 * to LDClientSDK_FlagNotifier_OnFlagChange.
 *
 * Create the struct, initialize the struct, set the FlagChanged handler
 * and optionally UserData, and then pass the struct to
 * LDClientSDK_FlagNotifier_OnFlagChange.
 *
 * @param listener Listener to initialize.
 */
LD_EXPORT(void) LDFlagListener_Init(struct LDFlagListener* listener);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
