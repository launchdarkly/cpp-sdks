#include <gtest/gtest.h>

#include <launchdarkly/server_side/bindings/c/integrations/redis/redis_source.h>

TEST(RedisBindings, SourcePointerIsStoredOnSuccessfulCreation) {
    LDServerLazyLoadResult result;
    ASSERT_TRUE(LDServerLazyLoadRedisSource_New("tcp://localhost:1234", "foo",
                                                &result));
    ASSERT_NE(result.source, nullptr);
    LDServerLazyLoadRedisSource_Free(result.source);
}

TEST(RedisBindings, ErrorMessageIsPropagatedOnFailure) {
    LDServerLazyLoadResult result;
    ASSERT_FALSE(
        LDServerLazyLoadRedisSource_New("totally not a URI", "foo", &result));
    // Note: this test might begin failing if the Redis++ library ever returns
    // a different string here. The important thing is not the exact message,
    // but that the message was propagated.
    ASSERT_STREQ(result.error_message, "invalid URI: no scheme");
}

TEST(RedisBindings, SourcePointerIsNullptrOnFailure) {
    LDServerLazyLoadResult result;
    ASSERT_FALSE(
        LDServerLazyLoadRedisSource_New("totally not a URI", "foo", &result));
    ASSERT_EQ(result.source, nullptr);
}
