// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include "c_bindings/status.h"

#include "error.hpp"

using namespace launchdarkly;

#define TO_ERROR(ptr) (reinterpret_cast<Error*>(ptr))

LD_EXPORT(char const*) LDStatus_Error(LDStatus res) {
    if (Error* e = TO_ERROR(res)) {
        return ErrorToString(*e);
    }
    return nullptr;
}

LD_EXPORT(bool) LDStatus_Ok(LDStatus res) {
    if (TO_ERROR(res) == nullptr) {
        return true;
    }
    return false;
}

LD_EXPORT(void) LDStatus_Free(LDStatus error) {
    if (Error* e = TO_ERROR(error)) {
        delete e;
    }
}

LD_EXPORT(LDStatus) LDStatus_Success(void) {
    return nullptr;
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
