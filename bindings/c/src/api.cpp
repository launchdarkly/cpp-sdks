#include <launchdarkly/api.h>
#include <launchdarkly/api.hpp>

bool launchdarkly_foo(int32_t* out_result) {
    if (auto val = launchdarkly::foo()) {
        *out_result = *val;
        return true;
    }
    return false;
}
