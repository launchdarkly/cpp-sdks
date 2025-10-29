#include <launchdarkly/server_side/bindings/c/hook_context.h>
#include <launchdarkly/server_side/hooks/hook.hpp>

#include <launchdarkly/detail/c_binding_helpers.hpp>

#define AS_HOOK_CONTEXT(ptr) \
    (reinterpret_cast<launchdarkly::server_side::hooks::HookContext*>(ptr))

using launchdarkly::server_side::hooks::HookContext;

LD_EXPORT(LDHookContext)
LDHookContext_New() {
    return reinterpret_cast<LDHookContext>(new HookContext());
}

LD_EXPORT(void)
LDHookContext_Set(LDHookContext hook_context,
                  char const* key,
                  void const* value) {
    LD_ASSERT(hook_context != nullptr);
    LD_ASSERT(key != nullptr);

    const auto shared_any = std::make_shared<std::any>(value);
    // The "any" wrapper will be allocated and deleted, but the contents
    // of the "any" will not.
    AS_HOOK_CONTEXT(hook_context)->Set(key, shared_any);
}

LD_EXPORT(bool)
LDHookContext_Get(LDHookContext hook_context,
                  char const* key,
                  void const** out_value) {
    LD_ASSERT(hook_context != nullptr);
    LD_ASSERT(key != nullptr);
    LD_ASSERT(out_value != nullptr);

    const auto result = AS_HOOK_CONTEXT(hook_context)->Get(key);
    if (result.has_value() && *result != nullptr) {
        try {
            *out_value = std::any_cast<const void*>(*result->get());
            return true;
        } catch (std::bad_any_cast const&) {
            // The stored value wasn't a void*, return false
            // Should not generally be possible.
        }
    }
    *out_value = nullptr;
    return false;
}

LD_EXPORT(void)
LDHookContext_Free(LDHookContext hook_context) {
    delete AS_HOOK_CONTEXT(hook_context);
}
