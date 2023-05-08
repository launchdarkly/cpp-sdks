#include <gtest/gtest.h>

#include "error.hpp"

#include "c_bindings/status.h"

TEST(StatusBindingTests, StatusOk) {
    LDStatus status = LDStatus_Success();
    ASSERT_TRUE(LDStatus_Ok(status));
    ASSERT_FALSE(LDStatus_Error(status));
    LDStatus_Free(status);
}

TEST(StatusBindingTests, StatusError) {
    using namespace launchdarkly;

    Error e = Error::kConfig_SDKKey_Empty;

    LDStatus status = reinterpret_cast<LDStatus>(new Error(e));

    ASSERT_FALSE(LDStatus_Ok(status));
    ASSERT_TRUE(LDStatus_Error(status));
    ASSERT_STREQ(LDStatus_Error(status), ErrorToString(e));
    LDStatus_Free(status);
}
