#include "gtest/gtest.h"

#include <launchdarkly/server_side/bindings/c/config/builder.h>
#include <launchdarkly/server_side/bindings/c/sdk.h>

#include <launchdarkly/server_side/bindings/c/hook.h>
#include <launchdarkly/server_side/bindings/c/hook_context.h>
#include <launchdarkly/server_side/bindings/c/evaluation_series_context.h>
#include <launchdarkly/server_side/bindings/c/evaluation_series_data.h>
#include <launchdarkly/server_side/bindings/c/track_series_context.h>

#include <launchdarkly/bindings/c/context_builder.h>

#include <launchdarkly/server_side/client.hpp>

#include <string>
#include <vector>

// Test tracker to verify hook execution
struct HookCallTracker {
    std::vector<std::string> calls;
    int before_eval_count = 0;
    int after_eval_count = 0;
    int after_track_count = 0;
};

// C callback functions for testing

static LDServerSDKEvaluationSeriesData TestHook_BeforeEvaluation(
    LDServerSDKEvaluationSeriesContext series_context,
    LDServerSDKEvaluationSeriesData data,
    void* user_data) {
    auto* tracker = static_cast<HookCallTracker*>(user_data);
    tracker->before_eval_count++;
    tracker->calls.push_back("before_eval");

    // Return the data unchanged for basic test
    return data;
}

static LDServerSDKEvaluationSeriesData TestHook_AfterEvaluation(
    LDServerSDKEvaluationSeriesContext series_context,
    LDServerSDKEvaluationSeriesData data,
    LDEvalDetail detail,
    void* user_data) {
    auto* tracker = static_cast<HookCallTracker*>(user_data);
    tracker->after_eval_count++;
    tracker->calls.push_back("after_eval");

    // Return the data unchanged for basic test
    return data;
}

static void TestHook_AfterTrack(
    LDServerSDKTrackSeriesContext track_context,
    void* user_data) {
    auto* tracker = static_cast<HookCallTracker*>(user_data);
    tracker->after_track_count++;
    tracker->calls.push_back("after_track");
}

// Test basic hook registration and execution
TEST(ServerCBindingsHooksTest, BasicHookExecution) {
    HookCallTracker tracker;

    struct LDServerSDKHook hook;
    LDServerSDKHook_Init(&hook);
    hook.Name = "TestHook";
    hook.BeforeEvaluation = TestHook_BeforeEvaluation;
    hook.AfterEvaluation = TestHook_AfterEvaluation;
    hook.AfterTrack = TestHook_AfterTrack;
    hook.UserData = &tracker;

    auto config_builder = LDServerConfigBuilder_New("sdk-key");
    LDServerConfigBuilder_Hooks(config_builder, hook);
    LDServerConfigBuilder_Events_Enabled(config_builder, false);

    LDServerConfig config = nullptr;
    LDServerConfigBuilder_Build(config_builder, &config);
    auto client = LDServerSDK_New(config);

    // Create a context for evaluation
    auto context_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(context_builder, "user", "user-key");
    auto context = LDContextBuilder_Build(context_builder);

    // Perform an evaluation - should trigger hooks
    LDServerSDK_BoolVariation(client, context, "test-flag", false);

    // Verify hooks were called
    EXPECT_EQ(tracker.before_eval_count, 1);
    EXPECT_EQ(tracker.after_eval_count, 1);
    EXPECT_EQ(tracker.calls.size(), 2);
    EXPECT_EQ(tracker.calls[0], "before_eval");
    EXPECT_EQ(tracker.calls[1], "after_eval");

    LDContext_Free(context);
    LDServerSDK_Free(client);
}

// Test track hooks
TEST(ServerCBindingsHooksTest, TrackHookExecution) {
    HookCallTracker tracker;

    struct LDServerSDKHook hook;
    LDServerSDKHook_Init(&hook);
    hook.Name = "TrackTestHook";
    hook.AfterTrack = TestHook_AfterTrack;
    hook.UserData = &tracker;

    auto config_builder = LDServerConfigBuilder_New("sdk-key");
    LDServerConfigBuilder_Hooks(config_builder, hook);
    LDServerConfigBuilder_Events_Enabled(config_builder, false);

    LDServerConfig config = nullptr;
    LDServerConfigBuilder_Build(config_builder, &config);
    auto client = LDServerSDK_New(config);

    // Create a context for tracking
    auto context_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(context_builder, "user", "user-key");
    auto context = LDContextBuilder_Build(context_builder);

    // Track an event - should trigger after_track hook
    LDServerSDK_TrackEvent(client, context, "test-event");

    // Verify hook was called
    EXPECT_EQ(tracker.after_track_count, 1);
    EXPECT_EQ(tracker.calls.size(), 1);
    EXPECT_EQ(tracker.calls[0], "after_track");

    LDContext_Free(context);
    LDServerSDK_Free(client);
}

// Hook that accesses evaluation series context accessors
static LDServerSDKEvaluationSeriesData ContextAccessor_BeforeEvaluation(
    LDServerSDKEvaluationSeriesContext series_context,
    LDServerSDKEvaluationSeriesData data,
    void* user_data) {
    auto* tracker = static_cast<HookCallTracker*>(user_data);

    // Test all accessor functions
    char const* flag_key = LDEvaluationSeriesContext_FlagKey(series_context);
    tracker->calls.push_back(std::string("flag:") + flag_key);

    LDContext context = LDEvaluationSeriesContext_Context(series_context);
    EXPECT_NE(context, nullptr);

    LDValue default_value = LDEvaluationSeriesContext_DefaultValue(series_context);
    EXPECT_NE(default_value, nullptr);

    char const* method = LDEvaluationSeriesContext_Method(series_context);
    tracker->calls.push_back(std::string("method:") + method);

    LDHookContext hook_context = LDEvaluationSeriesContext_HookContext(series_context);
    EXPECT_NE(hook_context, nullptr);

    char const* env_id = LDEvaluationSeriesContext_EnvironmentId(series_context);
    if (env_id) {
        tracker->calls.push_back(std::string("env:") + env_id);
    }

    return data;
}

TEST(ServerCBindingsHooksTest, EvaluationSeriesContextAccessors) {
    HookCallTracker tracker;

    struct LDServerSDKHook hook;
    LDServerSDKHook_Init(&hook);
    hook.Name = "ContextAccessorHook";
    hook.BeforeEvaluation = ContextAccessor_BeforeEvaluation;
    hook.UserData = &tracker;

    auto config_builder = LDServerConfigBuilder_New("sdk-key");
    LDServerConfigBuilder_Hooks(config_builder, hook);
    LDServerConfigBuilder_Events_Enabled(config_builder, false);

    LDServerConfig config = nullptr;
    LDServerConfigBuilder_Build(config_builder, &config);
    auto client = LDServerSDK_New(config);

    auto context_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(context_builder, "user", "user-key");
    auto context = LDContextBuilder_Build(context_builder);

    // Perform evaluation
    LDServerSDK_BoolVariation(client, context, "my-flag", false);

    // Verify accessor calls recorded expected values
    EXPECT_GE(tracker.calls.size(), 2);
    EXPECT_EQ(tracker.calls[0], "flag:my-flag");
    EXPECT_EQ(tracker.calls[1], "method:BoolVariation");

    LDContext_Free(context);
    LDServerSDK_Free(client);
}

// Hook that accesses track series context accessors
static void TrackContextAccessor_AfterTrack(
    LDServerSDKTrackSeriesContext track_context,
    void* user_data) {
    auto* tracker = static_cast<HookCallTracker*>(user_data);

    // Test all accessor functions
    char const* key = LDTrackSeriesContext_Key(track_context);
    tracker->calls.push_back(std::string("key:") + key);

    LDContext context = LDTrackSeriesContext_Context(track_context);
    EXPECT_NE(context, nullptr);

    LDValue data_value = nullptr;
    bool has_data = LDTrackSeriesContext_Data(track_context, &data_value);
    if (has_data) {
        tracker->calls.push_back("has_data");
    }

    double metric_value = 0.0;
    bool has_metric = LDTrackSeriesContext_MetricValue(track_context, &metric_value);
    if (has_metric) {
        tracker->calls.push_back("has_metric");
    }

    LDHookContext hook_context = LDTrackSeriesContext_HookContext(track_context);
    EXPECT_NE(hook_context, nullptr);

    char const* env_id = LDTrackSeriesContext_EnvironmentId(track_context);
    if (env_id) {
        tracker->calls.push_back(std::string("env:") + env_id);
    }
}

TEST(ServerCBindingsHooksTest, TrackSeriesContextAccessors) {
    HookCallTracker tracker;

    struct LDServerSDKHook hook;
    LDServerSDKHook_Init(&hook);
    hook.Name = "TrackContextAccessorHook";
    hook.AfterTrack = TrackContextAccessor_AfterTrack;
    hook.UserData = &tracker;

    auto config_builder = LDServerConfigBuilder_New("sdk-key");
    LDServerConfigBuilder_Hooks(config_builder, hook);
    LDServerConfigBuilder_Events_Enabled(config_builder, false);

    LDServerConfig config = nullptr;
    LDServerConfigBuilder_Build(config_builder, &config);
    auto client = LDServerSDK_New(config);

    auto context_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(context_builder, "user", "user-key");
    auto context = LDContextBuilder_Build(context_builder);

    // Track event
    LDServerSDK_TrackEvent(client, context, "test-event");

    // Verify accessor calls
    EXPECT_GE(tracker.calls.size(), 1);
    EXPECT_EQ(tracker.calls[0], "key:test-event");

    LDContext_Free(context);
    LDServerSDK_Free(client);
}

// Hook that passes data between stages using EvaluationSeriesData
static LDServerSDKEvaluationSeriesData DataPassing_BeforeEvaluation(
    LDServerSDKEvaluationSeriesContext series_context,
    LDServerSDKEvaluationSeriesData data,
    void* user_data) {
    // Create a builder from the incoming data
    auto builder = LDEvaluationSeriesData_NewBuilder(data);

    // Add a string value
    auto string_value = LDValue_NewString("test-string");
    LDEvaluationSeriesDataBuilder_SetValue(builder, "my-key", string_value);

    // Add a pointer value
    int* my_int = new int(42);
    LDEvaluationSeriesDataBuilder_SetPointer(builder, "my-pointer", my_int);

    // Store pointer in user_data so we can clean it up later
    auto* tracker = static_cast<HookCallTracker*>(user_data);
    tracker->calls.push_back("before:set_data");

    LDValue_Free(string_value);

    return LDEvaluationSeriesDataBuilder_Build(builder);
}

static LDServerSDKEvaluationSeriesData DataPassing_AfterEvaluation(
    LDServerSDKEvaluationSeriesContext series_context,
    LDServerSDKEvaluationSeriesData data,
    LDEvalDetail detail,
    void* user_data) {
    auto* tracker = static_cast<HookCallTracker*>(user_data);

    // Retrieve the string value
    LDValue retrieved_value = nullptr;
    bool found = LDEvaluationSeriesData_GetValue(data, "my-key", &retrieved_value);
    EXPECT_TRUE(found);
    if (found) {
        char const* str = LDValue_GetString(retrieved_value);
        EXPECT_STREQ(str, "test-string");
        tracker->calls.push_back("after:got_value");
    }

    // Retrieve the pointer
    void* retrieved_pointer = nullptr;
    found = LDEvaluationSeriesData_GetPointer(data, "my-pointer", &retrieved_pointer);
    EXPECT_TRUE(found);
    if (found) {
        int* my_int = static_cast<int*>(retrieved_pointer);
        EXPECT_EQ(*my_int, 42);
        tracker->calls.push_back("after:got_pointer");
        delete my_int;  // Clean up
    }

    return data;
}

TEST(ServerCBindingsHooksTest, DataPassingBetweenStages) {
    HookCallTracker tracker;

    struct LDServerSDKHook hook;
    LDServerSDKHook_Init(&hook);
    hook.Name = "DataPassingHook";
    hook.BeforeEvaluation = DataPassing_BeforeEvaluation;
    hook.AfterEvaluation = DataPassing_AfterEvaluation;
    hook.UserData = &tracker;

    auto config_builder = LDServerConfigBuilder_New("sdk-key");
    LDServerConfigBuilder_Hooks(config_builder, hook);
    LDServerConfigBuilder_Events_Enabled(config_builder, false);

    LDServerConfig config = nullptr;
    LDServerConfigBuilder_Build(config_builder, &config);
    auto client = LDServerSDK_New(config);

    auto context_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(context_builder, "user", "user-key");
    auto context = LDContextBuilder_Build(context_builder);

    // Perform evaluation
    LDServerSDK_BoolVariation(client, context, "test-flag", false);

    // Verify data was passed correctly
    EXPECT_EQ(tracker.calls.size(), 3);
    EXPECT_EQ(tracker.calls[0], "before:set_data");
    EXPECT_EQ(tracker.calls[1], "after:got_value");
    EXPECT_EQ(tracker.calls[2], "after:got_pointer");

    LDContext_Free(context);
    LDServerSDK_Free(client);
}

// Hook that uses HookContext to pass caller data
static LDServerSDKEvaluationSeriesData HookContextTest_BeforeEvaluation(
    LDServerSDKEvaluationSeriesContext series_context,
    LDServerSDKEvaluationSeriesData data,
    void* user_data) {
    auto* tracker = static_cast<HookCallTracker*>(user_data);

    LDHookContext hook_context = LDEvaluationSeriesContext_HookContext(series_context);

    // Try to get a value that should be set by the caller
    void const* retrieved_value = nullptr;
    bool found = LDHookContext_Get(hook_context, "span", &retrieved_value);
    if (found) {
        // In a real scenario, this would be an OpenTelemetry span
        int const* span_id = static_cast<int const*>(retrieved_value);
        tracker->calls.push_back(std::string("span:") + std::to_string(*span_id));
    }

    return data;
}

TEST(ServerCBindingsHooksTest, HookContextOperations) {
    HookCallTracker tracker;

    struct LDServerSDKHook hook;
    LDServerSDKHook_Init(&hook);
    hook.Name = "HookContextTestHook";
    hook.BeforeEvaluation = HookContextTest_BeforeEvaluation;
    hook.UserData = &tracker;

    auto config_builder = LDServerConfigBuilder_New("sdk-key");
    LDServerConfigBuilder_Hooks(config_builder, hook);
    LDServerConfigBuilder_Events_Enabled(config_builder, false);

    LDServerConfig config = nullptr;
    LDServerConfigBuilder_Build(config_builder, &config);
    auto client = LDServerSDK_New(config);

    auto context_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(context_builder, "user", "user-key");
    auto context = LDContextBuilder_Build(context_builder);

    // Create a HookContext and set a value (simulating caller setting context)
    auto hook_context = LDHookContext_New();
    int span_id = 12345;
    LDHookContext_Set(hook_context, "span", &span_id);

    // Perform evaluation with hook context
    // Note: The C API doesn't currently expose a way to pass HookContext to evaluations
    // In the real SDK, this would be passed through the evaluation call
    // For now, we'll test the HookContext operations independently

    // Test Get operation
    void const* retrieved_value = nullptr;
    bool found = LDHookContext_Get(hook_context, "span", &retrieved_value);
    EXPECT_TRUE(found);
    if (found) {
        int const* retrieved_span = static_cast<int const*>(retrieved_value);
        EXPECT_EQ(*retrieved_span, 12345);
    }

    // Test missing key
    found = LDHookContext_Get(hook_context, "missing-key", &retrieved_value);
    EXPECT_FALSE(found);

    LDHookContext_Free(hook_context);
    LDContext_Free(context);
    LDServerSDK_Free(client);
}

// Test multiple hooks executing in order
static LDServerSDKEvaluationSeriesData FirstHook_BeforeEvaluation(
    LDServerSDKEvaluationSeriesContext series_context,
    LDServerSDKEvaluationSeriesData data,
    void* user_data) {
    auto* tracker = static_cast<HookCallTracker*>(user_data);
    tracker->calls.push_back("first_hook");
    return data;
}

static LDServerSDKEvaluationSeriesData SecondHook_BeforeEvaluation(
    LDServerSDKEvaluationSeriesContext series_context,
    LDServerSDKEvaluationSeriesData data,
    void* user_data) {
    auto* tracker = static_cast<HookCallTracker*>(user_data);
    tracker->calls.push_back("second_hook");
    return data;
}

TEST(ServerCBindingsHooksTest, MultipleHooksInOrder) {
    HookCallTracker tracker;

    struct LDServerSDKHook hook1;
    LDServerSDKHook_Init(&hook1);
    hook1.Name = "FirstHook";
    hook1.BeforeEvaluation = FirstHook_BeforeEvaluation;
    hook1.UserData = &tracker;

    struct LDServerSDKHook hook2;
    LDServerSDKHook_Init(&hook2);
    hook2.Name = "SecondHook";
    hook2.BeforeEvaluation = SecondHook_BeforeEvaluation;
    hook2.UserData = &tracker;

    auto config_builder = LDServerConfigBuilder_New("sdk-key");
    LDServerConfigBuilder_Hooks(config_builder, hook1);
    LDServerConfigBuilder_Hooks(config_builder, hook2);
    LDServerConfigBuilder_Events_Enabled(config_builder, false);

    LDServerConfig config = nullptr;
    LDServerConfigBuilder_Build(config_builder, &config);
    auto client = LDServerSDK_New(config);

    auto context_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(context_builder, "user", "user-key");
    auto context = LDContextBuilder_Build(context_builder);

    // Perform evaluation
    LDServerSDK_BoolVariation(client, context, "test-flag", false);

    // Verify hooks executed in order
    EXPECT_GE(tracker.calls.size(), 2);
    EXPECT_EQ(tracker.calls[0], "first_hook");
    EXPECT_EQ(tracker.calls[1], "second_hook");

    LDContext_Free(context);
    LDServerSDK_Free(client);
}

// Test that NULL data is handled correctly
static LDServerSDKEvaluationSeriesData NullDataTest_BeforeEvaluation(
    LDServerSDKEvaluationSeriesContext series_context,
    LDServerSDKEvaluationSeriesData data,
    void* user_data) {
    auto* tracker = static_cast<HookCallTracker*>(user_data);

    // Return NULL to indicate no data
    tracker->calls.push_back("returned_null");
    return nullptr;
}

static LDServerSDKEvaluationSeriesData NullDataTest_AfterEvaluation(
    LDServerSDKEvaluationSeriesContext series_context,
    LDServerSDKEvaluationSeriesData data,
    LDEvalDetail detail,
    void* user_data) {
    auto* tracker = static_cast<HookCallTracker*>(user_data);

    // Should receive empty data from previous stage
    LDValue test_value = nullptr;
    bool found = LDEvaluationSeriesData_GetValue(data, "any-key", &test_value);
    EXPECT_FALSE(found);

    tracker->calls.push_back("received_empty");
    return data;
}

TEST(ServerCBindingsHooksTest, NullDataHandling) {
    HookCallTracker tracker;

    struct LDServerSDKHook hook;
    LDServerSDKHook_Init(&hook);
    hook.Name = "NullDataHook";
    hook.BeforeEvaluation = NullDataTest_BeforeEvaluation;
    hook.AfterEvaluation = NullDataTest_AfterEvaluation;
    hook.UserData = &tracker;

    auto config_builder = LDServerConfigBuilder_New("sdk-key");
    LDServerConfigBuilder_Hooks(config_builder, hook);
    LDServerConfigBuilder_Events_Enabled(config_builder, false);

    LDServerConfig config = nullptr;
    LDServerConfigBuilder_Build(config_builder, &config);
    auto client = LDServerSDK_New(config);

    auto context_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(context_builder, "user", "user-key");
    auto context = LDContextBuilder_Build(context_builder);

    // Perform evaluation
    LDServerSDK_BoolVariation(client, context, "test-flag", false);

    // Verify NULL was handled correctly
    EXPECT_EQ(tracker.calls.size(), 2);
    EXPECT_EQ(tracker.calls[0], "returned_null");
    EXPECT_EQ(tracker.calls[1], "received_empty");

    LDContext_Free(context);
    LDServerSDK_Free(client);
}

// Test EvaluationSeriesDataBuilder operations
TEST(ServerCBindingsHooksTest, EvaluationSeriesDataBuilder) {
    // Create new empty data
    auto data = LDEvaluationSeriesData_New();
    EXPECT_NE(data, nullptr);

    // Create builder from empty data
    auto builder = LDEvaluationSeriesData_NewBuilder(data);
    EXPECT_NE(builder, nullptr);

    // Add various values
    auto string_value = LDValue_NewString("test");
    LDEvaluationSeriesDataBuilder_SetValue(builder, "key1", string_value);

    auto number_value = LDValue_NewNumber(42.5);
    LDEvaluationSeriesDataBuilder_SetValue(builder, "key2", number_value);

    int my_int = 100;
    LDEvaluationSeriesDataBuilder_SetPointer(builder, "ptr1", &my_int);

    // Build new data
    auto new_data = LDEvaluationSeriesDataBuilder_Build(builder);
    EXPECT_NE(new_data, nullptr);

    // Verify values
    LDValue retrieved = nullptr;
    bool found = LDEvaluationSeriesData_GetValue(new_data, "key1", &retrieved);
    EXPECT_TRUE(found);
    if (found) {
        EXPECT_STREQ(LDValue_GetString(retrieved), "test");
    }

    found = LDEvaluationSeriesData_GetValue(new_data, "key2", &retrieved);
    EXPECT_TRUE(found);
    if (found) {
        EXPECT_DOUBLE_EQ(LDValue_GetNumber(retrieved), 42.5);
    }

    void* retrieved_ptr = nullptr;
    found = LDEvaluationSeriesData_GetPointer(new_data, "ptr1", &retrieved_ptr);
    EXPECT_TRUE(found);
    if (found) {
        int* ptr = static_cast<int*>(retrieved_ptr);
        EXPECT_EQ(*ptr, 100);
    }

    // Test missing key
    found = LDEvaluationSeriesData_GetValue(new_data, "missing", &retrieved);
    EXPECT_FALSE(found);

    LDValue_Free(string_value);
    LDValue_Free(number_value);
    LDEvaluationSeriesData_Free(new_data);
    LDEvaluationSeriesData_Free(data);
}

// Test that builder can be freed without building (error case)
TEST(ServerCBindingsHooksTest, EvaluationSeriesDataBuilderFreeWithoutBuild) {
    // Create a builder
    LDServerSDKEvaluationSeriesDataBuilder builder =
        LDEvaluationSeriesData_NewBuilder(nullptr);

    // Add some data to it
    LDValue test_value = LDValue_NewNumber(42.0);
    LDEvaluationSeriesDataBuilder_SetValue(builder, "test", test_value);

    int test_int = 100;
    LDEvaluationSeriesDataBuilder_SetPointer(builder, "ptr", &test_int);

    // Free the builder without building
    // This tests the error case where user decides not to proceed
    LDEvaluationSeriesDataBuilder_Free(builder);

    // No assertions needed - this test passes if no memory leaks occur
    // ASAN will detect any issues

    LDValue_Free(test_value);
}
