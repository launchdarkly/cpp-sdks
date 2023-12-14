// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include <launchdarkly/bindings/c/status.h>

#include <launchdarkly/error.hpp>

using namespace launchdarkly;

#define TO_STATUS(ptr) (reinterpret_cast<Error*>(ptr))

LD_EXPORT(char const*) LDStatus_Error(LDStatus const status) {
    if (Error const* e = TO_STATUS(status)) {
        return ErrorToString(*e);
    }
    return nullptr;
}

LD_EXPORT(bool) LDStatus_Ok(LDStatus const status) {
    if (TO_STATUS(status) == nullptr) {
        return true;
    }
    return false;
}

LD_EXPORT(void) LDStatus_Free(LDStatus const error) {
    delete TO_STATUS(error);
}

LD_EXPORT(LDStatus) LDStatus_Success() {
    return nullptr;
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
