#pragma once

namespace launchdarkly::detail {

// This may be replaced with a standard routine when C++23 is available.
[[noreturn]] inline void unreachable() {
// Uses compiler specific extensions if possible.
// Even if no extension is used, undefined behavior is still raised by
// an empty function body and the noreturn attribute.
#if defined(__GNUC__)  // GCC, Clang, ICC
    __builtin_unreachable();
#elif defined(_MSC_VER)  // MSVC
    __assume(false);
#endif
}
}  // namespace launchdarkly::detail
