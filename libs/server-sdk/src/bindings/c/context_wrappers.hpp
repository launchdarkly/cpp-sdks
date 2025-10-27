/**
 * @file context_wrappers.hpp
 * @brief Internal wrapper structures for hook contexts.
 *
 * These wrappers hold references to C++ context objects plus any temporary
 * values that need to remain alive during C callback execution.
 *
 * LIFETIME:
 * - Created on the stack in hook_wrapper.cpp before calling C callbacks
 * - Live for the entire duration of the C callback
 * - Automatically destroyed when callback returns
 */

#pragma once

#include <launchdarkly/server_side/hooks/hook.hpp>
#include <launchdarkly/value.hpp>

namespace launchdarkly::server_side::bindings {

/**
 * @brief Wrapper for EvaluationSeriesContext.
 *
 * Holds a reference to the C++ context.
 */
struct EvaluationSeriesContextWrapper {
    hooks::EvaluationSeriesContext const& context;

    explicit EvaluationSeriesContextWrapper(
        hooks::EvaluationSeriesContext const& ctx)
        : context(ctx) {}
};

/**
 * @brief Wrapper for TrackSeriesContext.
 *
 * Holds a reference to the C++ context.
 */
struct TrackSeriesContextWrapper {
    hooks::TrackSeriesContext const& context;

    explicit TrackSeriesContextWrapper(hooks::TrackSeriesContext const& ctx)
        : context(ctx) {}
};

}  // namespace launchdarkly::server_side::bindings
