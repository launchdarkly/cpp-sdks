#include <gtest/gtest.h>

#include <launchdarkly/error.hpp>

#include <launchdarkly/bindings/c/status.h>

// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast

TEST(StatusBindingTests, StatusOk) {
    LDStatus status = LDStatus_Success();
    ASSERT_TRUE(LDStatus_Ok(status));
    ASSERT_FALSE(LDStatus_Error(status));
    LDStatus_Free(status);
}

TEST(StatusBindingTests, StatusError) {
    using namespace launchdarkly;

    Error err = Error::kConfig_SDKKey_Empty;

    auto status = reinterpret_cast<LDStatus>(new Error(err));
    ASSERT_FALSE(LDStatus_Ok(status));
    ASSERT_TRUE(LDStatus_Error(status));
    ASSERT_STREQ(LDStatus_Error(status), ErrorToString(err));
    LDStatus_Free(status);
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
