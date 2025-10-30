/**
 * @file tracing_hook.hpp
 * @brief OpenTelemetry integration hook for LaunchDarkly C++ Server SDK
 *
 * This hook implements the OpenTelemetry integration specification for
 * LaunchDarkly, adding feature flag evaluation data to distributed traces.
 *
 * Specification: OTEL-opentelemetry-integration
 * Reference: https://github.com/launchdarkly/open-sdk-specs/specs/OTEL-opentelemetry-integration/
 */

#pragma once

#include <launchdarkly/server_side/hooks/hook.hpp>

#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/span.h>
#include <opentelemetry/trace/tracer.h>

#include <memory>
#include <optional>
#include <string>

namespace launchdarkly::server_side::integrations::otel {

// Forward declaration
class TracingHookOptionsBuilder;

/**
 * @brief Configuration options for the OpenTelemetry tracing hook
 *
 * This class is immutable. Use TracingHookOptionsBuilder to construct instances.
 */
class TracingHookOptions {
   public:
    /**
     * @brief Whether to include the flag evaluation result value in span events
     * @return true if values should be included
     */
    [[nodiscard]] bool IncludeValue() const { return include_value_; }

    /**
     * @brief Whether to create dedicated spans for each flag evaluation
     * @return true if dedicated spans should be created
     */
    [[nodiscard]] bool CreateSpans() const { return create_spans_; }

    /**
     * @brief Optional environment ID to include in telemetry
     * @return Environment ID if configured
     */
    [[nodiscard]] std::optional<std::string> const& EnvironmentId() const {
        return environment_id_;
    }

   private:
    friend class TracingHookOptionsBuilder;

    bool include_value_ = false;
    bool create_spans_ = false;
    std::optional<std::string> environment_id_;

    TracingHookOptions() = default;
};

/**
 * @brief Builder for TracingHookOptions
 *
 * @example Basic usage
 * ```cpp
 * auto options = launchdarkly::server_side::integrations::otel::TracingHookOptionsBuilder()
 *                    .IncludeValue(true)
 *                    .CreateSpans(false)
 *                    .Build();
 * auto hook = std::make_shared<launchdarkly::server_side::integrations::otel::TracingHook>(options);
 * ```
 *
 * @example With environment ID
 * ```cpp
 * auto options = launchdarkly::server_side::integrations::otel::TracingHookOptionsBuilder()
 *                    .IncludeValue(true)
 *                    .EnvironmentId("production")
 *                    .Build();
 * ```
 */
class TracingHookOptionsBuilder {
   public:
    /**
     * @brief Construct a builder with default options
     */
    TracingHookOptionsBuilder() = default;

    /**
     * @brief Set whether to include flag values in telemetry
     *
     * When enabled, the `feature_flag.result.value` attribute will be added
     * to span events with the evaluated flag value as a JSON string.
     *
     * @param include_value true to include values (default: false)
     * @return Reference to this builder for chaining
     */
    TracingHookOptionsBuilder& IncludeValue(bool include_value) {
        options_.include_value_ = include_value;
        return *this;
    }

    /**
     * @brief Set whether to create dedicated spans for evaluations
     *
     * When enabled, creates a new span for each flag evaluation with the name
     * format "LDClient.{method}" (e.g., "LDClient.BoolVariation").
     *
     * @param create_spans true to create spans (default: false)
     * @return Reference to this builder for chaining
     */
    TracingHookOptionsBuilder& CreateSpans(bool create_spans) {
        options_.create_spans_ = create_spans;
        return *this;
    }

    /**
     * @brief Set the environment ID to include in telemetry
     *
     * When provided, this will be used as the `feature_flag.set.id` attribute
     * in all span events.
     *
     * @param environment_id The LaunchDarkly environment ID
     * @return Reference to this builder for chaining
     */
    TracingHookOptionsBuilder& EnvironmentId(std::string environment_id) {
        if (!environment_id.empty()) {
            options_.environment_id_ = std::move(environment_id);
        }
        return *this;
    }

    /**
     * @brief Build the TracingHookOptions
     *
     * @return Configured TracingHookOptions instance
     */
    [[nodiscard]] TracingHookOptions Build() const { return options_; }

   private:
    TracingHookOptions options_;
};

/**
 * @brief OpenTelemetry tracing hook for LaunchDarkly feature flag evaluations
 *
 * ## Usage
 *
 * ### Basic Usage (Span Events Only)
 * ```cpp
 * auto hook = std::make_shared<launchdarkly::server_side::integrations::otel::TracingHook>();
 * auto config = launchdarkly::server_side::ConfigBuilder("sdk-key")
 *                   .Hooks(hook)
 *                   .Build()
 *                   .value();
 * launchdarkly::server_side::Client client(std::move(config));
 * ```
 *
 * ### Advanced Usage (With Options)
 * ```cpp
 * auto options = launchdarkly::server_side::integrations::otel::TracingHookOptionsBuilder()
 *                    .IncludeValue(true)
 *                    .CreateSpans(true)
 *                    .EnvironmentId("my-environment-id")
 *                    .Build();
 *
 * auto hook = std::make_shared<launchdarkly::server_side::integrations::otel::TracingHook>(options);
 * auto config = launchdarkly::server_side::ConfigBuilder("sdk-key")
 *                   .Hooks(hook)
 *                   .Build()
 *                   .value();
 * ```
 *
 * ### Providing a Parent Span via HookContext
 * ```cpp
 * // Get current OpenTelemetry span
 * auto current_span = opentelemetry::trace::Tracer::GetCurrentSpan();
 *
 * // Create hook context with parent span
 * launchdarkly::server_side::hooks::HookContext hook_ctx;
 * hook_ctx.Set("otel.span", std::make_shared<std::any>(current_span));
 *
 * // Evaluate with hook context
 * bool result = client.BoolVariation(context, "my-flag", false, hook_ctx);
 * ```
 */
class TracingHook : public hooks::Hook {
   public:
    /**
     * @brief Construct a tracing hook with default options
     */
    TracingHook();

    /**
     * @brief Construct a tracing hook with custom options
     * @param options Configuration options for the hook
     */
    explicit TracingHook(TracingHookOptions options);

    /**
     * @brief Get metadata about this hook
     * @return Hook metadata containing the hook name
     */
    [[nodiscard]] hooks::HookMetadata const& Metadata() const override;

    /**
     * @brief Stage executed before flag evaluation
     *
     * @param series_context Context information about the evaluation
     * @param data Series data from previous stages (empty for first stage)
     * @return Series data with span reference (if span creation enabled)
     */
    hooks::EvaluationSeriesData BeforeEvaluation(
        hooks::EvaluationSeriesContext const& series_context,
        hooks::EvaluationSeriesData data) override;

    /**
     * @brief Stage executed after flag evaluation
     *
     * @param series_context Context information about the evaluation
     * @param data Series data from BeforeEvaluation stage
     * @param detail The evaluation result detail
     * @return Series data (unchanged)
     */
    hooks::EvaluationSeriesData AfterEvaluation(
        hooks::EvaluationSeriesContext const& series_context,
        hooks::EvaluationSeriesData data,
        EvaluationDetail<Value> const& detail) override;

   private:
    /**
     * @brief Get the active OpenTelemetry span
     *
     * @param hook_ctx Hook context that may contain a parent span
     * @return Shared pointer to the active span, or nullptr if none exists
     */
    [[nodiscard]] static opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span>
    GetActiveSpan(hooks::HookContext const& hook_ctx);

    /**
     * @brief Get the tracer for creating new spans
     * @return Shared pointer to the tracer
     */
    [[nodiscard]] static opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer>
    GetTracer();

    /**
     * @brief Add a feature_flag event to a span
     *
     * @param span The span to add the event to
     * @param series_context Context with flag key and evaluation context
     * @param detail Evaluation result with value and variation index
     */
    void AddFeatureFlagEvent(
        opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> const&
            span,
        hooks::EvaluationSeriesContext const& series_context,
        EvaluationDetail<Value> const& detail) const;

    /**
     * @brief Get the environment ID to use in telemetry
     *
     * @param series_context Context that may contain environment ID
     * @return Environment ID if available
     */
    [[nodiscard]] std::optional<std::string> GetEnvironmentId(
        hooks::EvaluationSeriesContext const& series_context) const;

    TracingHookOptions options_;
    hooks::HookMetadata metadata_;
};

/**
 * @brief Helper function to create a HookContext from an OpenTelemetry span
 *
 * This convenience function simplifies passing the current span to flag
 * evaluation methods, ensuring that feature flag events are added to the
 * correct span in the trace hierarchy.
 *
 * @param span The OpenTelemetry span to attach to the HookContext
 * @return HookContext configured with the provided span
 *
 * @example
 * ```cpp
 * auto span = tracer->StartSpan("handle_request");
 * auto hook_ctx = launchdarkly::server_side::integrations::otel::MakeHookContextWithSpan(span);
 * bool result = client.BoolVariation(context, "my-flag", false, hook_ctx);
 * span->End();
 * ```
 */
inline hooks::HookContext MakeHookContextWithSpan(
    const opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span>& span) {
    hooks::HookContext ctx;
    ctx.Set("otel.span", std::make_shared<std::any>(span));
    return ctx;
}

}  // namespace launchdarkly::server_side::integrations::otel
