#include <launchdarkly/server_side/bindings/c/hook.h>

#include <launchdarkly/detail/c_binding_helpers.hpp>

#include <cstring>

LD_EXPORT(void)
LDServerSDKHook_Init(LDServerSDKHook* hook) {
    LD_ASSERT(hook != nullptr);

    hook->Name = nullptr;
    hook->BeforeEvaluation = nullptr;
    hook->AfterEvaluation = nullptr;
    hook->AfterTrack = nullptr;
    hook->UserData = nullptr;
}
