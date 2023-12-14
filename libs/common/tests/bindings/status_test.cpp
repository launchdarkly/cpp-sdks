#include <gtest/gtest.h>

#include "launchdarkly/error.hpp"

#include "launchdarkly/bindings/c/status.h"

// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
using namespace launchdarkly;

TEST(StatusBindingTests, StatusOk) {
    LDStatus status = LDStatus_Success();
    ASSERT_TRUE(LDStatus_Ok(status));
    ASSERT_FALSE(LDStatus_Error(status));
    LDStatus_Free(status);
}

TEST(StatusBindingTests, StatusErrorCode) {
    Error err = ErrorCode::kConfig_SDKKey_Empty;

    auto status = reinterpret_cast<LDStatus>(new Error(err));
    ASSERT_FALSE(LDStatus_Ok(status));
    ASSERT_TRUE(LDStatus_Error(status));
    ASSERT_STREQ(LDStatus_Error(status), ErrorToString(err));
    LDStatus_Free(status);
}

TEST(StatusBindingTests, StatusErrorString) {
    auto status = reinterpret_cast<LDStatus>(new Error(
        "this is an arbitrary error"));
    ASSERT_FALSE(LDStatus_Ok(status));
    ASSERT_TRUE(LDStatus_Error(status));
    ASSERT_STRCASEEQ(LDStatus_Error(status),
                     "this is an arbitrary error");
    LDStatus_Free(status);
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
