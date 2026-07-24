#include <gtest/gtest.h>

#include <launchdarkly/server_side/bindings/c/integrations/redis/redis_big_segment_store.h>
#include <launchdarkly/server_side/bindings/c/integrations/redis/redis_source.h>

#include <launchdarkly/bindings/c/context_builder.h>
#include <launchdarkly/server_side/bindings/c/config/builder.h>
#include <launchdarkly/server_side/bindings/c/sdk.h>

#include <launchdarkly/data_model/flag.hpp>

#include "prefixed_redis_client.hpp"

#include <chrono>
#include <condition_variable>
#include <mutex>

using namespace launchdarkly::data_model;

TEST(RedisBindings, SourcePointerIsStoredOnSuccessfulCreation) {
    LDServerLazyLoadRedisResult result;
    ASSERT_TRUE(LDServerLazyLoadRedisSource_New("tcp://localhost:1234", "foo",
                                                &result));
    ASSERT_NE(result.source, nullptr);
    LDServerLazyLoadRedisSource_Free(result.source);
}

TEST(RedisBindings, ErrorMessageIsPropagatedOnFailure) {
    LDServerLazyLoadRedisResult result;
    ASSERT_FALSE(
        LDServerLazyLoadRedisSource_New("totally not a URI", "foo", &result));
    // Note: this test might begin failing if the Redis++ library ever returns
    // a different string here. The important thing is not the exact message,
    // but that the message was propagated.
    ASSERT_STREQ(result.error_message, "invalid URI: no scheme");
}

TEST(RedisBindings, SourcePointerIsNullptrOnFailure) {
    LDServerLazyLoadRedisResult result;
    ASSERT_FALSE(
        LDServerLazyLoadRedisSource_New("totally not a URI", "foo", &result));
    ASSERT_EQ(result.source, nullptr);
}

// This is an end-to-end test that uses an actual Redis instance with
// provisioned flag data. The source is passed into the SDK's LazyLoad data
// system, and then AllFlags is used to verify that the data is read back from
// Redis correctly.
TEST(RedisBindings, CanUseInSDKLazyLoadDataSource) {
    sw::redis::Redis redis("tcp://localhost:6379");
    redis.flushdb();

    PrefixedClient client(redis, "testprefix");
    Flag flag_a{"foo", 1, false, std::nullopt, {true, false}};
    flag_a.offVariation = 0;  // variation: true
    Flag flag_b{"bar", 1, false, std::nullopt, {true, false}};
    flag_b.offVariation = 1;  // variation: false

    client.PutFlag(flag_a);
    client.PutFlag(flag_b);
    client.Init();

    LDServerLazyLoadRedisResult result;
    ASSERT_TRUE(LDServerLazyLoadRedisSource_New("tcp://localhost:6379",
                                                "testprefix", &result));

    LDServerConfigBuilder cfg_builder = LDServerConfigBuilder_New("sdk-123");

    LDServerLazyLoadBuilder lazy_builder = LDServerLazyLoadBuilder_New();
    LDServerLazyLoadBuilder_SourcePtr(
        lazy_builder,
        reinterpret_cast<LDServerLazyLoadSourcePtr>(result.source));
    LDServerConfigBuilder_DataSystem_LazyLoad(cfg_builder, lazy_builder);
    LDServerConfigBuilder_Events_Enabled(
        cfg_builder, false);  // Don't want outbound connection to
    // LD in test.

    LDServerConfig config;
    LDStatus status = LDServerConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    LDServerSDK sdk = LDServerSDK_New(config);
    LDServerSDK_Start(sdk, LD_NONBLOCKING, nullptr);

    LDContextBuilder ctx_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(ctx_builder, "cat", "shadow");
    LDContext context = LDContextBuilder_Build(ctx_builder);

    LDAllFlagsState state =
        LDServerSDK_AllFlagsState(sdk, context, LD_ALLFLAGSSTATE_DEFAULT);

    ASSERT_TRUE(LDAllFlagsState_Valid(state));
    LDValue all = LDAllFlagsState_Map(state);

    ASSERT_EQ(LDValue_Type(all), LDValueType_Object);

    std::unordered_map<std::string, launchdarkly::Value> values;
    LDValue_ObjectIter iter;
    for (iter = LDValue_ObjectIter_New(all); !LDValue_ObjectIter_End(iter);
         LDValue_ObjectIter_Next(iter)) {
        char const* key = LDValue_ObjectIter_Key(iter);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto value_ref = reinterpret_cast<launchdarkly::Value const* const>(
            LDValue_ObjectIter_Value(iter));
        values.emplace(key, *value_ref);
    }

    LDValue_ObjectIter_Free(iter);

    std::unordered_map<std::string, launchdarkly::Value> expected = {
        {"foo", true}, {"bar", false}};
    ASSERT_EQ(values, expected);

    LDValue_Free(all);
    LDAllFlagsState_Free(state);

    LDContext_Free(context);
    LDServerSDK_Free(sdk);
}

TEST(RedisBindings, BigSegmentsStorePointerIsStoredOnSuccessfulCreation) {
    LDServerBigSegmentsRedisResult result;
    ASSERT_TRUE(LDServerBigSegmentsRedisStore_New("tcp://localhost:1234", "foo",
                                                  &result));
    ASSERT_NE(result.store, nullptr);
    LDServerBigSegmentsRedisStore_Free(result.store);
}

TEST(RedisBindings, BigSegmentsErrorMessageIsPropagatedOnFailure) {
    LDServerBigSegmentsRedisResult result;
    ASSERT_FALSE(
        LDServerBigSegmentsRedisStore_New("totally not a URI", "foo", &result));
    ASSERT_STRNE(result.error_message, "");
}

TEST(RedisBindings, BigSegmentsStorePointerIsNullptrOnFailure) {
    LDServerBigSegmentsRedisResult result;
    ASSERT_FALSE(
        LDServerBigSegmentsRedisStore_New("totally not a URI", "foo", &result));
    ASSERT_EQ(result.store, nullptr);
}

// This is an end-to-end test that uses an actual Redis instance with
// provisioned Big Segments metadata. The store is passed into the SDK's Big
// Segments configuration, and the SDK's Big Segment store status listener is
// used to verify that the store is reachable and reports available.
TEST(RedisBindings, CanUseInSDKBigSegmentsConfig) {
    sw::redis::Redis redis("tcp://localhost:6379");
    redis.flushdb();

    // Set the store's sync timestamp so the poll reports available.
    auto const now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
    redis.set("testprefix:big_segments_synchronized_on",
              std::to_string(now_ms));

    LDServerBigSegmentsRedisResult result;
    ASSERT_TRUE(LDServerBigSegmentsRedisStore_New("tcp://localhost:6379",
                                                  "testprefix", &result));

    LDServerBigSegmentsBuilder bs_builder = LDServerBigSegmentsBuilder_New(
        reinterpret_cast<LDServerBigSegmentStorePtr>(result.store));
    LDServerBigSegmentsBuilder_StatusPollIntervalMs(bs_builder, 50);

    LDServerConfigBuilder cfg_builder = LDServerConfigBuilder_New("sdk-123");
    LDServerConfigBuilder_BigSegments(cfg_builder, bs_builder);
    LDServerConfigBuilder_Offline(cfg_builder, true);

    LDServerConfig config;
    LDStatus status = LDServerConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    LDServerSDK sdk = LDServerSDK_New(config);

    std::mutex mu;
    std::condition_variable cv;
    bool available = false;

    struct ListenerCtx {
        std::mutex* mu;
        std::condition_variable* cv;
        bool* available;
    };
    ListenerCtx ctx{&mu, &cv, &available};

    struct LDServerBigSegmentStoreStatusListener listener{};
    LDServerBigSegmentStoreStatusListener_Init(&listener);
    listener.UserData = &ctx;
    listener.StatusChanged =
        +[](LDServerBigSegmentStoreStatus s, void* user_data) {
            auto* c = static_cast<ListenerCtx*>(user_data);
            {
                std::lock_guard<std::mutex> lk(*c->mu);
                *c->available = LDServerBigSegmentStoreStatus_Available(s);
            }
            c->cv->notify_all();
        };

    LDListenerConnection connection =
        LDServerSDK_BigSegmentStoreStatus_OnStatusChange(sdk, listener);
    ASSERT_NE(connection, nullptr);

    LDServerSDK_Start(sdk, LD_NONBLOCKING, nullptr);

    {
        std::unique_lock<std::mutex> lk(mu);
        cv.wait_for(lk, std::chrono::seconds(3), [&] { return available; });
    }

    EXPECT_TRUE(available);

    LDListenerConnection_Disconnect(connection);
    LDListenerConnection_Free(connection);
    LDServerSDK_Free(sdk);
}
