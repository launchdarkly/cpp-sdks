// Tests for hooks implementation based on:
// https://github.com/launchdarkly/sdk-specs/blob/main/specs/HOOK-hooks/README.md
// Spec commit: main branch as of implementation date
//
// This test file validates the hooks specification requirements for server-side SDKs.

#include <gtest/gtest.h>
#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/server_side/client.hpp>
#include <launchdarkly/server_side/config/config_builder.hpp>
#include <launchdarkly/server_side/hooks/hook.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace launchdarkly;
using namespace launchdarkly::server_side;
using namespace launchdarkly::server_side::hooks;

// Test hook that records all method calls for verification
class RecordingHook : public Hook {
   public:
    struct EvaluationCall {
        std::string stage;  // "before" or "after"
        std::string flag_key;
        std::string method;
        std::optional<Value> result_value;
    };

    struct TrackCall {
        std::string key;
        std::optional<double> metric_value;
    };

    explicit RecordingHook(std::string name) : metadata_(std::move(name)) {}

    HookMetadata const& Metadata() const override { return metadata_; }

    EvaluationSeriesData BeforeEvaluation(
        EvaluationSeriesContext const& series_context,
        EvaluationSeriesData data) override {
        EvaluationCall call;
        call.stage = "before";
        call.flag_key = std::string(series_context.FlagKey());
        call.method = std::string(series_context.Method());
        evaluation_calls_.push_back(call);

        // Add some data to pass to after stage
        EvaluationSeriesDataBuilder builder(data);
        builder.Set("hook_name", Value(std::string(metadata_.Name())));
        builder.Set("call_count", Value(static_cast<int>(evaluation_calls_.size())));
        return builder.Build();
    }

    EvaluationSeriesData AfterEvaluation(
        EvaluationSeriesContext const& series_context,
        EvaluationSeriesData data,
        EvaluationDetail<Value> const& detail) override {
        EvaluationCall call;
        call.stage = "after";
        call.flag_key = std::string(series_context.FlagKey());
        call.method = std::string(series_context.Method());
        call.result_value = detail.Value();
        evaluation_calls_.push_back(call);

        // Verify we can read the data from before stage
        auto hook_name = data.Get("hook_name");
        if (hook_name) {
            received_data_from_before_ = true;
        }

        return data;
    }

    void AfterTrack(TrackSeriesContext const& series_context) override {
        TrackCall call;
        call.key = std::string(series_context.Key());
        call.metric_value = series_context.MetricValue();
        track_calls_.push_back(call);
    }

    std::vector<EvaluationCall> const& GetEvaluationCalls() const {
        return evaluation_calls_;
    }

    std::vector<TrackCall> const& GetTrackCalls() const {
        return track_calls_;
    }

    bool ReceivedDataFromBefore() const { return received_data_from_before_; }

    void Reset() {
        evaluation_calls_.clear();
        track_calls_.clear();
        received_data_from_before_ = false;
    }

   private:
    HookMetadata metadata_;
    std::vector<EvaluationCall> evaluation_calls_;
    std::vector<TrackCall> track_calls_;
    bool received_data_from_before_ = false;
};

// Hook that throws exceptions to test error handling
class ErrorThrowingHook : public Hook {
   public:
    enum class ThrowStage { Before, After, Track, None };

    ErrorThrowingHook(std::string name, ThrowStage throw_stage)
        : metadata_(std::move(name)), throw_stage_(throw_stage) {}

    HookMetadata const& Metadata() const override { return metadata_; }

    EvaluationSeriesData BeforeEvaluation(
        EvaluationSeriesContext const& series_context,
        EvaluationSeriesData data) override {
        if (throw_stage_ == ThrowStage::Before) {
            throw std::runtime_error("BeforeEvaluation error");
        }
        return data;
    }

    EvaluationSeriesData AfterEvaluation(
        EvaluationSeriesContext const& series_context,
        EvaluationSeriesData data,
        EvaluationDetail<Value> const& detail) override {
        if (throw_stage_ == ThrowStage::After) {
            throw std::runtime_error("AfterEvaluation error");
        }
        return data;
    }

    void AfterTrack(TrackSeriesContext const& series_context) override {
        if (throw_stage_ == ThrowStage::Track) {
            throw std::runtime_error("AfterTrack error");
        }
    }

   private:
    HookMetadata metadata_;
    ThrowStage throw_stage_;
};

class HooksTest : public ::testing::Test {
   protected:
    HooksTest()
        : context_(ContextBuilder().Kind("user", "test-user").Build()) {}

    Context const context_;
};

// Requirement 1.1.3: A hook MUST provide a getMetadata method
TEST_F(HooksTest, HookProvidesMetadata) {
    auto hook = std::make_shared<RecordingHook>("TestHook");
    ASSERT_EQ(hook->Metadata().Name(), "TestHook");
}

// Requirement 1.2.1: Hooks MUST support a beforeEvaluation stage
TEST_F(HooksTest, BeforeEvaluationStageExecuted) {
    auto hook = std::make_shared<RecordingHook>("TestHook");

    auto config = ConfigBuilder("sdk-key")
                      .Offline(true)
                      .Hooks(hook)
                      .Build()
                      .value();

    Client client(std::move(config));
    client.BoolVariation(context_, "test-flag", false);

    auto const& calls = hook->GetEvaluationCalls();
    ASSERT_EQ(calls.size(), 2);
    EXPECT_EQ(calls[0].stage, "before");
    EXPECT_EQ(calls[0].flag_key, "test-flag");
}

// Requirement 1.2.2: Hooks MUST support an afterEvaluation stage
TEST_F(HooksTest, AfterEvaluationStageExecuted) {
    auto hook = std::make_shared<RecordingHook>("TestHook");

    auto config = ConfigBuilder("sdk-key")
                      .Offline(true)
                      .Hooks(hook)
                      .Build()
                      .value();

    Client client(std::move(config));
    client.BoolVariation(context_, "test-flag", true);

    auto const& calls = hook->GetEvaluationCalls();
    ASSERT_EQ(calls.size(), 2);
    EXPECT_EQ(calls[1].stage, "after");
    EXPECT_EQ(calls[1].flag_key, "test-flag");
    EXPECT_TRUE(calls[1].result_value.has_value());
}

// Requirement 1.2.2.3: The afterEvaluation stage MUST be executed with the
// EvaluationSeriesData returned by the previous stage
TEST_F(HooksTest, SeriesDataPropagatedBetweenStages) {
    auto hook = std::make_shared<RecordingHook>("TestHook");

    auto config = ConfigBuilder("sdk-key")
                      .Offline(true)
                      .Hooks(hook)
                      .Build()
                      .value();

    Client client(std::move(config));
    client.BoolVariation(context_, "test-flag", false);

    EXPECT_TRUE(hook->ReceivedDataFromBefore());
}

// Requirement 1.3.1: The client MUST provide a mechanism of registering hooks
// during initialization
TEST_F(HooksTest, HooksRegisteredDuringInitialization) {
    auto hook1 = std::make_shared<RecordingHook>("Hook1");
    auto hook2 = std::make_shared<RecordingHook>("Hook2");

    auto config = ConfigBuilder("sdk-key")
                      .Offline(true)
                      .Hooks(hook1)
                      .Hooks(hook2)
                      .Build()
                      .value();

    Client client(std::move(config));
    client.BoolVariation(context_, "test-flag", false);

    EXPECT_EQ(hook1->GetEvaluationCalls().size(), 2);
    EXPECT_EQ(hook2->GetEvaluationCalls().size(), 2);
}

// Requirement 1.3.4: The client MUST execute hooks in the following order:
// - beforeEvaluation: in order of registration
// - afterEvaluation: in reverse order of registration
TEST_F(HooksTest, HooksExecutedInCorrectOrder) {
    auto hook1 = std::make_shared<RecordingHook>("Hook1");
    auto hook2 = std::make_shared<RecordingHook>("Hook2");
    auto hook3 = std::make_shared<RecordingHook>("Hook3");

    // To verify order, we'll use a shared vector
    auto execution_order = std::make_shared<std::vector<std::string>>();

    class OrderTrackingHook : public Hook {
       public:
        OrderTrackingHook(std::string name,
                          std::shared_ptr<std::vector<std::string>> order)
            : metadata_(std::move(name)), order_(std::move(order)) {}

        HookMetadata const& Metadata() const override { return metadata_; }

        EvaluationSeriesData BeforeEvaluation(
            EvaluationSeriesContext const& series_context,
            EvaluationSeriesData data) override {
            order_->push_back(std::string(metadata_.Name()) + "-before");
            return data;
        }

        EvaluationSeriesData AfterEvaluation(
            EvaluationSeriesContext const& series_context,
            EvaluationSeriesData data,
            EvaluationDetail<Value> const& detail) override {
            order_->push_back(std::string(metadata_.Name()) + "-after");
            return data;
        }

       private:
        HookMetadata metadata_;
        std::shared_ptr<std::vector<std::string>> order_;
    };

    auto order_hook1 =
        std::make_shared<OrderTrackingHook>("Hook1", execution_order);
    auto order_hook2 =
        std::make_shared<OrderTrackingHook>("Hook2", execution_order);
    auto order_hook3 =
        std::make_shared<OrderTrackingHook>("Hook3", execution_order);

    auto config = ConfigBuilder("sdk-key")
                      .Offline(true)
                      .Hooks(order_hook1)
                      .Hooks(order_hook2)
                      .Hooks(order_hook3)
                      .Build()
                      .value();

    Client client(std::move(config));
    client.BoolVariation(context_, "test-flag", false);

    // Expected order: Hook1-before, Hook2-before, Hook3-before,
    //                 Hook3-after, Hook2-after, Hook1-after
    ASSERT_EQ(execution_order->size(), 6);
    EXPECT_EQ((*execution_order)[0], "Hook1-before");
    EXPECT_EQ((*execution_order)[1], "Hook2-before");
    EXPECT_EQ((*execution_order)[2], "Hook3-before");
    EXPECT_EQ((*execution_order)[3], "Hook3-after");
    EXPECT_EQ((*execution_order)[4], "Hook2-after");
    EXPECT_EQ((*execution_order)[5], "Hook1-after");
}

// Requirement 1.3.6: The client MUST support propagation of series data
// between stages in a series for a single invocation
TEST_F(HooksTest, SeriesDataIsolatedPerHook) {
    class DataIsolationHook : public Hook {
       public:
        explicit DataIsolationHook(std::string name, std::string marker)
            : metadata_(std::move(name)), marker_(std::move(marker)) {}

        HookMetadata const& Metadata() const override { return metadata_; }

        EvaluationSeriesData BeforeEvaluation(
            EvaluationSeriesContext const& series_context,
            EvaluationSeriesData data) override {
            EvaluationSeriesDataBuilder builder(data);
            builder.Set("marker", Value(marker_));
            return builder.Build();
        }

        EvaluationSeriesData AfterEvaluation(
            EvaluationSeriesContext const& series_context,
            EvaluationSeriesData data,
            EvaluationDetail<Value> const& detail) override {
            auto marker = data.Get("marker");
            EXPECT_TRUE(marker.has_value());
            if (marker) {
                EXPECT_EQ(*marker, Value(marker_));
                received_correct_marker_ = true;
            }
            return data;
        }

        bool ReceivedCorrectMarker() const { return received_correct_marker_; }

       private:
        HookMetadata metadata_;
        std::string marker_;
        bool received_correct_marker_ = false;
    };

    auto hook1 = std::make_shared<DataIsolationHook>("Hook1", "marker1");
    auto hook2 = std::make_shared<DataIsolationHook>("Hook2", "marker2");

    auto config = ConfigBuilder("sdk-key")
                      .Offline(true)
                      .Hooks(hook1)
                      .Hooks(hook2)
                      .Build()
                      .value();

    Client client(std::move(config));
    client.BoolVariation(context_, "test-flag", false);

    // Each hook should have received its own marker, not the other's
    EXPECT_TRUE(hook1->ReceivedCorrectMarker());
    EXPECT_TRUE(hook2->ReceivedCorrectMarker());
}

// Requirement 1.3.7: The client MUST handle exceptions which are thrown during
// the execution of a stage, allowing operations to complete unaffected
TEST_F(HooksTest, ExceptionsInBeforeEvaluationDoNotAffectEvaluation) {
    auto error_hook = std::make_shared<ErrorThrowingHook>(
        "ErrorHook", ErrorThrowingHook::ThrowStage::Before);
    auto recording_hook = std::make_shared<RecordingHook>("RecordingHook");

    auto config = ConfigBuilder("sdk-key")
                      .Offline(true)
                      .Hooks(error_hook)
                      .Hooks(recording_hook)
                      .Build()
                      .value();

    Client client(std::move(config));

    // Evaluation should still complete and return default value
    bool result = client.BoolVariation(context_, "test-flag", true);
    EXPECT_EQ(result, true);

    // Recording hook should still have been called
    EXPECT_EQ(recording_hook->GetEvaluationCalls().size(), 2);
}

// Requirement 1.3.7: Exceptions in afterEvaluation
TEST_F(HooksTest, ExceptionsInAfterEvaluationDoNotAffectEvaluation) {
    auto error_hook = std::make_shared<ErrorThrowingHook>(
        "ErrorHook", ErrorThrowingHook::ThrowStage::After);
    auto recording_hook = std::make_shared<RecordingHook>("RecordingHook");

    auto config = ConfigBuilder("sdk-key")
                      .Offline(true)
                      .Hooks(error_hook)
                      .Hooks(recording_hook)
                      .Build()
                      .value();

    Client client(std::move(config));

    // Evaluation should still complete and return default value
    bool result = client.BoolVariation(context_, "test-flag", true);
    EXPECT_EQ(result, true);

    // Recording hook should still have been called
    EXPECT_EQ(recording_hook->GetEvaluationCalls().size(), 2);
}

// Requirement 1.6.1: Hooks MUST support a afterTrack handler
TEST_F(HooksTest, AfterTrackHandlerExecuted) {
    auto hook = std::make_shared<RecordingHook>("TestHook");

    auto config = ConfigBuilder("sdk-key")
                      .Offline(true)
                      .Hooks(hook)
                      .Build()
                      .value();

    Client client(std::move(config));
    client.Track(context_, "test-event");

    auto const& calls = hook->GetTrackCalls();
    ASSERT_EQ(calls.size(), 1);
    EXPECT_EQ(calls[0].key, "test-event");
}

// Requirement 1.6.1: afterTrack with metric value
TEST_F(HooksTest, AfterTrackWithMetricValue) {
    auto hook = std::make_shared<RecordingHook>("TestHook");

    auto config = ConfigBuilder("sdk-key")
                      .Offline(true)
                      .Hooks(hook)
                      .Build()
                      .value();

    Client client(std::move(config));
    client.Track(context_, "test-event", Value::Null(), 42.5);

    auto const& calls = hook->GetTrackCalls();
    ASSERT_EQ(calls.size(), 1);
    EXPECT_EQ(calls[0].key, "test-event");
    ASSERT_TRUE(calls[0].metric_value.has_value());
    EXPECT_EQ(*calls[0].metric_value, 42.5);
}

// Requirement 1.3.4: afterTrack handlers execute in order of registration
TEST_F(HooksTest, AfterTrackExecutesInOrder) {
    auto execution_order = std::make_shared<std::vector<std::string>>();

    class OrderTrackingTrackHook : public Hook {
       public:
        OrderTrackingTrackHook(std::string name,
                               std::shared_ptr<std::vector<std::string>> order)
            : metadata_(std::move(name)), order_(std::move(order)) {}

        HookMetadata const& Metadata() const override { return metadata_; }

        void AfterTrack(TrackSeriesContext const& series_context) override {
            order_->push_back(std::string(metadata_.Name()));
        }

       private:
        HookMetadata metadata_;
        std::shared_ptr<std::vector<std::string>> order_;
    };

    auto hook1 =
        std::make_shared<OrderTrackingTrackHook>("Hook1", execution_order);
    auto hook2 =
        std::make_shared<OrderTrackingTrackHook>("Hook2", execution_order);
    auto hook3 =
        std::make_shared<OrderTrackingTrackHook>("Hook3", execution_order);

    auto config = ConfigBuilder("sdk-key")
                      .Offline(true)
                      .Hooks(hook1)
                      .Hooks(hook2)
                      .Hooks(hook3)
                      .Build()
                      .value();

    Client client(std::move(config));
    client.Track(context_, "test-event");

    // Expected order: Hook1, Hook2, Hook3
    ASSERT_EQ(execution_order->size(), 3);
    EXPECT_EQ((*execution_order)[0], "Hook1");
    EXPECT_EQ((*execution_order)[1], "Hook2");
    EXPECT_EQ((*execution_order)[2], "Hook3");
}

// Test that exceptions in afterTrack don't affect tracking
TEST_F(HooksTest, ExceptionsInAfterTrackDoNotAffectTracking) {
    auto error_hook = std::make_shared<ErrorThrowingHook>(
        "ErrorHook", ErrorThrowingHook::ThrowStage::Track);
    auto recording_hook = std::make_shared<RecordingHook>("RecordingHook");

    auto config = ConfigBuilder("sdk-key")
                      .Offline(true)
                      .Hooks(error_hook)
                      .Hooks(recording_hook)
                      .Build()
                      .value();

    Client client(std::move(config));
    client.Track(context_, "test-event");

    // Recording hook should still have been called despite error in first hook
    EXPECT_EQ(recording_hook->GetTrackCalls().size(), 1);
}

// Test evaluation context provides correct information
TEST_F(HooksTest, EvaluationContextProvidesCorrectInformation) {
    class ContextVerifyingHook : public Hook {
       public:
        explicit ContextVerifyingHook(std::string name)
            : metadata_(std::move(name)) {}

        HookMetadata const& Metadata() const override { return metadata_; }

        EvaluationSeriesData BeforeEvaluation(
            EvaluationSeriesContext const& series_context,
            EvaluationSeriesData data) override {
            flag_key_ = std::string(series_context.FlagKey());
            method_ = std::string(series_context.Method());
            default_value_ = series_context.DefaultValue();
            has_context_ = series_context.EvaluationContext().Valid();
            return data;
        }

        std::string flag_key_;
        std::string method_;
        Value default_value_;
        bool has_context_ = false;

       private:
        HookMetadata metadata_;
    };

    auto hook = std::make_shared<ContextVerifyingHook>("TestHook");

    auto config = ConfigBuilder("sdk-key")
                      .Offline(true)
                      .Hooks(hook)
                      .Build()
                      .value();

    Client client(std::move(config));
    client.StringVariation(context_, "test-flag", "default-value");

    EXPECT_EQ(hook->flag_key_, "test-flag");
    EXPECT_EQ(hook->method_, "Variation");
    EXPECT_EQ(hook->default_value_, Value("default-value"));
    EXPECT_TRUE(hook->has_context_);
}

// Test that Variation vs VariationDetail methods are correctly identified
TEST_F(HooksTest, MethodNameDistinguishesVariationTypes) {
    class MethodVerifyingHook : public Hook {
       public:
        explicit MethodVerifyingHook(std::string name)
            : metadata_(std::move(name)) {}

        HookMetadata const& Metadata() const override { return metadata_; }

        EvaluationSeriesData BeforeEvaluation(
            EvaluationSeriesContext const& series_context,
            EvaluationSeriesData data) override {
            methods_.push_back(std::string(series_context.Method()));
            return data;
        }

        std::vector<std::string> methods_;

       private:
        HookMetadata metadata_;
    };

    auto hook = std::make_shared<MethodVerifyingHook>("TestHook");

    auto config = ConfigBuilder("sdk-key")
                      .Offline(true)
                      .Hooks(hook)
                      .Build()
                      .value();

    Client client(std::move(config));
    client.BoolVariation(context_, "test-flag", false);
    client.BoolVariationDetail(context_, "test-flag", false);

    ASSERT_EQ(hook->methods_.size(), 2);
    EXPECT_EQ(hook->methods_[0], "Variation");
    EXPECT_EQ(hook->methods_[1], "VariationDetail");
}

// Test EvaluationSeriesData builder functionality
TEST_F(HooksTest, EvaluationSeriesDataBuilder) {
    EvaluationSeriesDataBuilder builder;
    builder.Set("key1", Value("value1"));
    builder.Set("key2", Value(42));
    builder.Set("key3", Value(true));

    auto data = builder.Build();

    EXPECT_TRUE(data.Has("key1"));
    EXPECT_TRUE(data.Has("key2"));
    EXPECT_TRUE(data.Has("key3"));
    EXPECT_FALSE(data.Has("nonexistent"));

    auto value1 = data.Get("key1");
    ASSERT_TRUE(value1.has_value());
    EXPECT_EQ(*value1, Value("value1"));

    auto value2 = data.Get("key2");
    ASSERT_TRUE(value2.has_value());
    EXPECT_EQ(*value2, Value(42));

    auto keys = data.Keys();
    EXPECT_EQ(keys.size(), 3);
}

// Test that series data can be built from existing data
TEST_F(HooksTest, EvaluationSeriesDataBuilderFromExisting) {
    EvaluationSeriesDataBuilder initial_builder;
    initial_builder.Set("existing", Value("data"));
    auto initial_data = initial_builder.Build();

    EvaluationSeriesDataBuilder builder(initial_data);
    builder.Set("new", Value("value"));

    auto data = builder.Build();

    EXPECT_TRUE(data.Has("existing"));
    EXPECT_TRUE(data.Has("new"));

    auto existing = data.Get("existing");
    ASSERT_TRUE(existing.has_value());
    EXPECT_EQ(*existing, Value("data"));

    auto new_val = data.Get("new");
    ASSERT_TRUE(new_val.has_value());
    EXPECT_EQ(*new_val, Value("value"));
}

// Test storing and retrieving shared_ptr<any> for arbitrary objects like spans
TEST_F(HooksTest, EvaluationSeriesDataStoresSharedPtr) {
    // Simulate a span object
    struct MockSpan {
        std::string trace_id = "trace-123";
        bool closed = false;
    };

    EvaluationSeriesDataBuilder builder;
    auto span = std::make_shared<std::any>(MockSpan{});
    builder.SetShared("span", span);
    builder.Set("string_value", Value("test"));

    auto data = builder.Build();

    EXPECT_TRUE(data.Has("span"));
    EXPECT_TRUE(data.Has("string_value"));

    // Retrieve the span
    auto retrieved_span = data.GetShared("span");
    ASSERT_TRUE(retrieved_span.has_value());

    // Cast back to the original type
    auto typed_span = std::any_cast<MockSpan>(*(*retrieved_span));
    EXPECT_EQ(typed_span.trace_id, "trace-123");
    EXPECT_FALSE(typed_span.closed);

    // Verify we can still get regular values
    auto string_val = data.Get("string_value");
    ASSERT_TRUE(string_val.has_value());
    EXPECT_EQ(*string_val, Value("test"));
}

// Test that Get returns nullopt for shared_ptr keys
TEST_F(HooksTest, EvaluationSeriesDataGetReturnsNulloptForSharedKeys) {
    struct MockSpan {};

    EvaluationSeriesDataBuilder builder;
    auto span = std::make_shared<std::any>(MockSpan{});
    builder.SetShared("span", span);

    auto data = builder.Build();

    // Get should return nullopt for shared_ptr keys
    auto value = data.Get("span");
    EXPECT_FALSE(value.has_value());

    // GetShared should work
    auto shared = data.GetShared("span");
    EXPECT_TRUE(shared.has_value());
}

// Test that GetShared returns nullopt for Value keys
TEST_F(HooksTest, EvaluationSeriesDataGetSharedReturnsNulloptForValueKeys) {
    EvaluationSeriesDataBuilder builder;
    builder.Set("value", Value(42));

    auto data = builder.Build();

    // GetShared should return nullopt for Value keys
    auto shared = data.GetShared("value");
    EXPECT_FALSE(shared.has_value());

    // Get should work
    auto value = data.Get("value");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, Value(42));
}

// Test span use case: create in beforeEvaluation, close in afterEvaluation
TEST_F(HooksTest, SpanLifecycleAcrossEvaluationStages) {
    struct MockSpan {
        std::string trace_id;
        bool closed = false;

        void Close() { closed = true; }
    };

    class SpanHook : public Hook {
       public:
        explicit SpanHook(std::string name) : metadata_(std::move(name)) {}

        HookMetadata const& Metadata() const override { return metadata_; }

        EvaluationSeriesData BeforeEvaluation(
            EvaluationSeriesContext const& series_context,
            EvaluationSeriesData data) override {
            auto span = std::make_shared<std::any>(
                MockSpan{"trace-" + std::string(series_context.FlagKey())});

            EvaluationSeriesDataBuilder builder(data);
            builder.SetShared("span", span);
            return builder.Build();
        }

        EvaluationSeriesData AfterEvaluation(
            EvaluationSeriesContext const& series_context,
            EvaluationSeriesData data,
            EvaluationDetail<Value> const& detail) override {
            auto span_any = data.GetShared("span");
            if (span_any) {
                auto& span = std::any_cast<MockSpan&>(*(*span_any));
                span.Close();
                span_closed_ = span.closed;
            }
            return data;
        }

        bool span_closed_ = false;

       private:
        HookMetadata metadata_;
    };

    auto hook = std::make_shared<SpanHook>("SpanHook");

    auto config = ConfigBuilder("sdk-key")
                      .Offline(true)
                      .Hooks(hook)
                      .Build()
                      .value();

    Client client(std::move(config));
    client.BoolVariation(context_, "test-flag", false);

    EXPECT_TRUE(hook->span_closed_);
}

// Test that shared_ptr allows mutation of the stored object
TEST_F(HooksTest, SharedPtrAllowsMutationAcrossStages) {
    struct Counter {
        int count = 0;
    };

    class CounterHook : public Hook {
       public:
        explicit CounterHook(std::string name) : metadata_(std::move(name)) {}

        HookMetadata const& Metadata() const override { return metadata_; }

        EvaluationSeriesData BeforeEvaluation(
            EvaluationSeriesContext const& series_context,
            EvaluationSeriesData data) override {
            auto counter = std::make_shared<std::any>(Counter{});

            // Mutate the counter
            auto& c = std::any_cast<Counter&>(*counter);
            c.count = 10;

            EvaluationSeriesDataBuilder builder(data);
            builder.SetShared("counter", counter);
            return builder.Build();
        }

        EvaluationSeriesData AfterEvaluation(
            EvaluationSeriesContext const& series_context,
            EvaluationSeriesData data,
            EvaluationDetail<Value> const& detail) override {
            auto counter_any = data.GetShared("counter");
            if (counter_any) {
                // Verify we can still mutate it
                auto& counter = std::any_cast<Counter&>(*(*counter_any));
                counter.count += 5;
                final_count_ = counter.count;
            }
            return data;
        }

        int final_count_ = 0;

       private:
        HookMetadata metadata_;
    };

    auto hook = std::make_shared<CounterHook>("CounterHook");

    auto config = ConfigBuilder("sdk-key")
                      .Offline(true)
                      .Hooks(hook)
                      .Build()
                      .value();

    Client client(std::move(config));
    client.BoolVariation(context_, "test-flag", false);

    EXPECT_EQ(hook->final_count_, 15);
}

// Test HookContext for passing caller data to hooks (e.g., OpenTelemetry span parents)
TEST_F(HooksTest, HookContextBasicFunctionality) {
    HookContext ctx;

    // Set values
    auto span_parent = std::make_shared<std::any>(std::string("span-parent-123"));
    auto trace_id = std::make_shared<std::any>(42);

    ctx.Set("span_parent", span_parent);
    ctx.Set("trace_id", trace_id);

    // Check existence
    EXPECT_TRUE(ctx.Has("span_parent"));
    EXPECT_TRUE(ctx.Has("trace_id"));
    EXPECT_FALSE(ctx.Has("nonexistent"));

    // Retrieve values
    auto retrieved_span = ctx.Get("span_parent");
    ASSERT_TRUE(retrieved_span.has_value());
    auto span_str = std::any_cast<std::string>(*(*retrieved_span));
    EXPECT_EQ(span_str, "span-parent-123");

    auto retrieved_trace = ctx.Get("trace_id");
    ASSERT_TRUE(retrieved_trace.has_value());
    auto trace_val = std::any_cast<int>(*(*retrieved_trace));
    EXPECT_EQ(trace_val, 42);
}

// Test that hooks can access HookContext from EvaluationSeriesContext
// This simulates the OpenTelemetry span parent use case
TEST_F(HooksTest, HookAccessesCallerProvidedContext) {
    struct SpanContext {
        std::string trace_id;
        std::string parent_span_id;
    };

    class OTelHook : public Hook {
       public:
        explicit OTelHook(std::string name) : metadata_(std::move(name)) {}

        HookMetadata const& Metadata() const override { return metadata_; }

        EvaluationSeriesData BeforeEvaluation(
            EvaluationSeriesContext const& series_context,
            EvaluationSeriesData data) override {
            // Access span parent from caller's context
            auto span_parent_any = series_context.HookCtx().Get("span_parent");
            if (span_parent_any) {
                auto span_parent =
                    std::any_cast<SpanContext>(*(*span_parent_any));
                received_trace_id_ = span_parent.trace_id;
                received_parent_span_ = span_parent.parent_span_id;

                // Create new span as child
                auto new_span = std::make_shared<std::any>(SpanContext{
                    span_parent.trace_id, "child-span-" + received_parent_span_});

                EvaluationSeriesDataBuilder builder(data);
                builder.SetShared("span", new_span);
                return builder.Build();
            }
            return data;
        }

        EvaluationSeriesData AfterEvaluation(
            EvaluationSeriesContext const& series_context,
            EvaluationSeriesData data,
            EvaluationDetail<Value> const& detail) override {
            // Close the span
            auto span_any = data.GetShared("span");
            if (span_any) {
                auto span = std::any_cast<SpanContext>(*(*span_any));
                closed_span_trace_ = span.trace_id;
            }
            return data;
        }

        std::string received_trace_id_;
        std::string received_parent_span_;
        std::string closed_span_trace_;

       private:
        HookMetadata metadata_;
    };

    auto hook = std::make_shared<OTelHook>("OTelHook");

    auto config = ConfigBuilder("sdk-key")
                      .Offline(true)
                      .Hooks(hook)
                      .Build()
                      .value();

    Client client(std::move(config));

    // Create a HookContext with span parent information (simulating OpenTelemetry)
    HookContext hook_context;
    SpanContext span_parent{"trace-123", "parent-span-456"};
    hook_context.Set("span_parent",
                     std::make_shared<std::any>(span_parent));

    // Call variation with the HookContext
    client.BoolVariation(context_, "test-flag", false, hook_context);

    // Verify hook received the span parent information
    EXPECT_EQ(hook->received_trace_id_, "trace-123");
    EXPECT_EQ(hook->received_parent_span_, "parent-span-456");
    EXPECT_EQ(hook->closed_span_trace_, "trace-123");
}

// Test HookContext chaining
TEST_F(HooksTest, HookContextChaining) {
    HookContext ctx;
    auto val1 = std::make_shared<std::any>(1);
    auto val2 = std::make_shared<std::any>(2);

    ctx.Set("key1", val1).Set("key2", val2);

    EXPECT_TRUE(ctx.Has("key1"));
    EXPECT_TRUE(ctx.Has("key2"));

    auto retrieved1 = ctx.Get("key1");
    ASSERT_TRUE(retrieved1.has_value());
    EXPECT_EQ(std::any_cast<int>(*(*retrieved1)), 1);

    auto retrieved2 = ctx.Get("key2");
    ASSERT_TRUE(retrieved2.has_value());
    EXPECT_EQ(std::any_cast<int>(*(*retrieved2)), 2);
}
