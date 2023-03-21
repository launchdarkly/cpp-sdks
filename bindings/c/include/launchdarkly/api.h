#pragma once

#include <launchdarkly/export.h>

#ifndef __cplusplus
#include <stdbool.h>
#include <stdint.h>
#else
#include <cstdint>
#endif

#ifdef __cplusplus
extern "C" {
#endif

LD_EXPORT(bool) launchdarkly_foo(int32_t* out_result);

#ifdef __cplusplus
}
#endif
