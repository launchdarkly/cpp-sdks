#include "launchdarkly/client_side/api.hpp"
#include <launchdarkly/api.h>

bool launchdarkly_foo(int32_t* out_result) {
    if (auto val = launchdarkly::foo()) {
        *out_result = *val;
        return true;
    }
    return false;
}
