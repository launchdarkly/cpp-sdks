#include <launchdarkly/bindings/c/flag_listener.h>

LD_EXPORT(void) LDFlagListener_Init(struct LDFlagListener* listener) {
    listener->FlagChanged = nullptr;
    listener->UserData = nullptr;
}
