/**
 * @file hook_wrapper.hpp
 * @brief C++ wrapper that bridges C hook callbacks to C++ Hook interface.
 *
 * This internal class captures C callback function pointers and user data,
 * then forwards hook method calls from the C++ Hook interface to the C callbacks.
 *
 * OWNERSHIP MODEL:
 * - The wrapper is created as a shared_ptr and registered with ConfigBuilder
 * - The wrapper copies the Name string during construction
 * - UserData lifetime is managed by the C application
 * - Function pointers are copied and assumed valid for the SDK lifetime
 */

#pragma once

#include <launchdarkly/server_side/bindings/c/hook.h>
#include <launchdarkly/server_side/hooks/hook.hpp>

#include <memory>
#include <string>

namespace launchdarkly::server_side::bindings {

/**
 * @brief Wrapper that adapts C hook callbacks to C++ Hook interface.
 *
 * This class implements the C++ Hook interface and forwards all calls
 * to the C callback functions provided by the application.
 */
class CHookWrapper final : public hooks::Hook {
   public:
    /**
     * @brief Construct a hook wrapper from C hook struct.
     *
     * @param c_hook C hook structure containing callbacks and metadata.
     *               The Name string is copied. UserData pointer and function
     *               pointers are copied but the pointed-to data lifetime is
     *               managed by the caller.
     */
    explicit CHookWrapper(struct LDServerSDKHook const& c_hook);

    /**
     * @brief Get hook metadata.
     */
    [[nodiscard]] hooks::HookMetadata const& Metadata() const override;

    /**
     * @brief Forward beforeEvaluation to C callback if set.
     */
    hooks::EvaluationSeriesData BeforeEvaluation(
        hooks::EvaluationSeriesContext const& series_context,
        hooks::EvaluationSeriesData data) override;

    /**
     * @brief Forward afterEvaluation to C callback if set.
     */
    hooks::EvaluationSeriesData AfterEvaluation(
        hooks::EvaluationSeriesContext const& series_context,
        hooks::EvaluationSeriesData data,
        EvaluationDetail<Value> const& detail) override;

    /**
     * @brief Forward afterTrack to C callback if set.
     */
    void AfterTrack(hooks::TrackSeriesContext const& series_context) override;

   private:
    hooks::HookMetadata metadata_;
    LDServerSDKHook_BeforeEvaluation before_evaluation_;
    LDServerSDKHook_AfterEvaluation after_evaluation_;
    LDServerSDKHook_AfterTrack after_track_;
    void* user_data_;
};

}  // namespace launchdarkly::server_side::bindings
