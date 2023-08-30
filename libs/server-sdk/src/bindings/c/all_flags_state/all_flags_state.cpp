#include <launchdarkly/server_side/bindings/c/all_flags_state/all_flags_state.h>

#include <launchdarkly/server_side/all_flags_state.hpp>

#define TO_ALLFLAGS(ptr) (reinterpret_cast<AllFlagsState*>(ptr))
#define FROM_ALLFLAGS(ptr) (reinterpret_cast<LDAllFlagsState>(ptr))

using namespace launchdarkly::server_side;

LD_EXPORT(void) LDAllFlagsState_Free(LDAllFlagsState state) {
    delete TO_ALLFLAGS(state);
}

LD_EXPORT(bool) LDAllFlagsState_Valid(LDAllFlagsState state) {
    return TO_ALLFLAGS(state)->Valid();
}
