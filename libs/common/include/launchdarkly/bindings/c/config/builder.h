/** @file
 * This file exists solely for backwards compatibility purposes
 * and will be removed in the next major release.
 *
 * Please use 'launchdarkly/bindings/c/config/client_side/builder.h' instead.
 *
 * The original C++ Client-side 1.0 release shipped builder.h at this location,
 * but it was moved to the client_side subdirectory for distinction with the C++
 * Server-side SDK. */
#pragma once

#include <launchdarkly/bindings/c/config/client_side/builder.h>
#pragma message( \
        "LaunchDarkly Client-side C++ SDK: The config/builder.h header is being removed in the next major version")
#pragma message( \
        "LaunchDarkly Client-side C++ SDK: Please use config/client_side/builder.h instead")
