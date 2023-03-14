#include <launchdarkly/api.h>
#include <stdint.h>
#include <stdio.h>

int main() {
    int32_t out;
    if (launchdarkly_foo(&out)) {
        printf("Got: %d\n", out);
    } else {
        printf("Got nothing\n");
    }
    return 0;
}
