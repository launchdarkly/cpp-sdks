#include "hook_wrapper.hpp"
#include "context_wrappers.hpp"

#include <launchdarkly/bindings/c/data/evaluation_detail.h>
#include <launchdarkly/bindings/c/value.h>
#include <launchdarkly/context.hpp>

#include <cassert>

// Helper macros for type conversions
#define AS_EVAL_SERIES_CONTEXT_WRAPPER(ptr) \
    (reinterpret_cast<LDServerSDKEvaluationSeriesContext>(ptr))

#define AS_EVAL_SERIES_DATA(ptr) \
    (reinterpret_cast<LDServerSDKEvaluationSeriesData>( \
        &const_cast<launchdarkly::server_side::hooks::EvaluationSeriesData&>(ptr)))

#define AS_TRACK_SERIES_CONTEXT_WRAPPER(ptr) \
    (reinterpret_cast<LDServerSDKTrackSeriesContext>(ptr))

#define AS_EVAL_DETAIL(ptr) \
    (reinterpret_cast<LDEvalDetail>( \
        const_cast<launchdarkly::EvaluationDetail<launchdarkly::Value>*>(ptr)))

#define AS_CPP_EVAL_SERIES_DATA(ptr) \
    (reinterpret_cast<launchdarkly::server_side::hooks::EvaluationSeriesData*>(ptr))

namespace launchdarkly::server_side::bindings {

CHookWrapper::CHookWrapper(struct LDServerSDKHook const& c_hook)
    : metadata_(c_hook.Name ? c_hook.Name : ""),
      before_evaluation_(c_hook.BeforeEvaluation),
      after_evaluation_(c_hook.AfterEvaluation),
      after_track_(c_hook.AfterTrack),
      user_data_(c_hook.UserData) {
    assert(c_hook.Name != nullptr && "Hook name must not be NULL");
}

hooks::HookMetadata const& CHookWrapper::Metadata() const {
    return metadata_;
}

hooks::EvaluationSeriesData CHookWrapper::BeforeEvaluation(
    hooks::EvaluationSeriesContext const& series_context,
    hooks::EvaluationSeriesData data) {
    // If no callback is set, return the data unmodified
    if (!before_evaluation_) {
        return data;
    }

    // Create wrapper on stack - holds context reference + default value copy
    EvaluationSeriesContextWrapper wrapper(series_context);

    // Convert to C types
    const auto c_series_context =
        AS_EVAL_SERIES_CONTEXT_WRAPPER(&wrapper);

    // Create a heap-allocated copy of the data to pass to C callback
    // This gives the callback ownership that it can return or modify
    const auto c_data_input =
        reinterpret_cast<LDServerSDKEvaluationSeriesData>(
            new hooks::EvaluationSeriesData(data));

    // Call the C callback - wrapper stays alive for entire call
    LDServerSDKEvaluationSeriesData result_data =
        before_evaluation_(c_series_context, c_data_input, user_data_);

    // Convert result back to C++
    if (result_data) {
        // Check if callback returned a different pointer than input
        // If so, we need to free the unused input
        if (result_data != c_data_input) {
            delete AS_CPP_EVAL_SERIES_DATA(c_data_input);
        }

        // Take ownership of the returned data
        hooks::EvaluationSeriesData cpp_result =
            std::move(*AS_CPP_EVAL_SERIES_DATA(result_data));
        // Free the C wrapper (but not the contents, which were moved)
        delete AS_CPP_EVAL_SERIES_DATA(result_data);
        return cpp_result;
    }

    // If NULL was returned, free the input data and return empty data
    delete AS_CPP_EVAL_SERIES_DATA(c_data_input);
    return {};
}

hooks::EvaluationSeriesData CHookWrapper::AfterEvaluation(
    hooks::EvaluationSeriesContext const& series_context,
    hooks::EvaluationSeriesData data,
    EvaluationDetail<Value> const& detail) {
    // If no callback is set, return the data unmodified
    if (!after_evaluation_) {
        return data;
    }

    // Create wrapper on stack - holds context reference + default value copy
    EvaluationSeriesContextWrapper wrapper(series_context);

    // Convert to C types
    const auto c_series_context =
        AS_EVAL_SERIES_CONTEXT_WRAPPER(&wrapper);

    // Create a heap-allocated copy of the data to pass to C callback
    // This gives the callback ownership that it can return or modify
    const auto c_data_input =
        reinterpret_cast<LDServerSDKEvaluationSeriesData>(
            new hooks::EvaluationSeriesData(data));

    const auto c_detail = AS_EVAL_DETAIL(&detail);

    // Call the C callback - wrapper stays alive for entire call
    LDServerSDKEvaluationSeriesData result_data =
        after_evaluation_(c_series_context, c_data_input, c_detail, user_data_);

    // Convert result back to C++
    if (result_data) {
        // Check if callback returned a different pointer than input
        // If so, we need to free the unused input
        if (result_data != c_data_input) {
            delete AS_CPP_EVAL_SERIES_DATA(c_data_input);
        }

        // Take ownership of the returned data
        hooks::EvaluationSeriesData cpp_result =
            std::move(*AS_CPP_EVAL_SERIES_DATA(result_data));
        // Free the C wrapper (but not the contents, which were moved)
        delete AS_CPP_EVAL_SERIES_DATA(result_data);
        return cpp_result;
    }

    // If NULL was returned, free the input data and return empty data
    delete AS_CPP_EVAL_SERIES_DATA(c_data_input);
    return {};
}

void CHookWrapper::AfterTrack(
    hooks::TrackSeriesContext const& series_context) {
    // If no callback is set, do nothing
    if (!after_track_) {
        return;
    }

    // Create wrapper on stack - holds context reference
    TrackSeriesContextWrapper wrapper(series_context);

    // Convert to C type
    const auto c_series_context =
        AS_TRACK_SERIES_CONTEXT_WRAPPER(&wrapper);

    // Call the C callback - wrapper stays alive for entire call
    after_track_(c_series_context, user_data_);
}

}  // namespace launchdarkly::server_side::bindings
