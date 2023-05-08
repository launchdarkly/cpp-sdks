// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include "c_bindings/error.h"

#include "error.hpp"

using namespace launchdarkly;

#define TO_ERROR(ptr) (reinterpret_cast<Error*>(ptr))

LD_EXPORT(char const*) LDError_ToString(LDError error) {
    return ErrorToString(*TO_ERROR(error));
}

LD_EXPORT(void) LDError_Free(LDError error) {
    delete TO_ERROR(error);
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
