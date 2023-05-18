#include <launchdarkly/bindings/c/memory_routines.h>
#include <cstdlib>

LD_EXPORT(void)
LDMemory_FreeString(char* string) {
    free(string);
}
