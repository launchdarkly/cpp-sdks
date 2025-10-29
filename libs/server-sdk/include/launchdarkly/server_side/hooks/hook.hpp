#pragma once

#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_detail.hpp>
#include <launchdarkly/value.hpp>

#include <any>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace launchdarkly::server_side::hooks {

/**
 * HookContext allows passing arbitrary data from the caller through to hooks.
 *
 * Example use case:
 * @code
 * // Caller creates context with span parent
 * HookContext ctx;
 * ctx.Set("otel_span_parent", std::make_shared<std::any>(span_context));
 *
 * // Pass to variation method
 * client.BoolVariation(context, "flag-key", false, ctx);
 *
 * // Hook accesses the span parent
 * auto span_parent = hook_context.Get("otel_span_parent");
 * @endcode
 */
class HookContext {
   public:
    /**
     * Constructs an empty HookContext.
     */
    HookContext() = default;

    /**
     * Sets a value in the context.
     * @param key The key to set.
     * @param value The shared_ptr to any type to associate with the key.
     * @return Reference to this context for chaining.
     */
    HookContext& Set(std::string key, std::shared_ptr<std::any> value);

    /**
     * Retrieves a value from the context.
     * @param key The key to look up.
     * @return The shared_ptr if present, or std::nullopt if not found.
     */
    [[nodiscard]] std::optional<std::shared_ptr<std::any>> Get(
        std::string const& key) const;

    /**
     * Checks if a key exists in the context.
     * @param key The key to check.
     * @return True if the key exists, false otherwise.
     */
    [[nodiscard]] bool Has(std::string const& key) const;

   private:
    std::map<std::string, std::shared_ptr<std::any>> data_;
};

/**
 * Metadata about a hook implementation.
 *
 * Lifetime: Objects of this type are owned by the hook
 * and remain valid for the lifetime of the hook.
 */
class HookMetadata {
   public:
    /**
     * Constructs hook metadata.
     * @param name The name of the hook.
     */
    explicit HookMetadata(std::string name);

    /**
     * Returns the name of the hook.
     *
     * Lifetime: The returned string_view is valid only for the lifetime
     * of this metadata object. If you need the name beyond the immediate
     * call, copy it to a std::string.
     *
     * @return The hook name as a string_view (immutable).
     */
    [[nodiscard]] std::string_view Name() const;

   private:
    std::string name_;
};

/**
 * Immutable data that can be passed between hook stages.
 * Provides readonly access to data passed between hook stages.
 *
 * Supports two types of data:
 * - Value: LaunchDarkly Value types (strings, numbers, booleans, etc.)
 * - std::shared_ptr<std::any>: Arbitrary objects (e.g., OpenTelemetry spans)
 *
 * Lifetime: Objects of this type are valid only during
 * the execution of a hook stage. Do not store references or pointers
 * to this data. If you need data beyond the stage execution, copy it.
 */
class EvaluationSeriesData {
   public:
    /**
     * Constructs empty series data.
     */
    EvaluationSeriesData();

    /**
     * Retrieves a Value from the series data.
     *
     * Lifetime: The returned reference (if present) is valid only during
     * the execution of the current hook stage. If you need the value
     * beyond this call, make a copy.
     *
     * @param key The key to look up.
     * @return Reference to the value if present, or std::nullopt if not found
     * or if the key maps to a shared_ptr.
     */
    [[nodiscard]] std::optional<std::reference_wrapper<Value const>> Get(std::string const& key) const;

    /**
     * Retrieves a shared_ptr to any type from the series data.
     *
     * Use this for storing arbitrary objects like OpenTelemetry spans
     * that need to be passed from beforeEvaluation to afterEvaluation.
     *
     * Example:
     * @code
     * auto span = data.GetShared("span");
     * if (span) {
     *     auto typed_span = std::any_cast<MySpanType>(*span);
     * }
     * @endcode
     *
     * @param key The key to look up.
     * @return The shared_ptr if present, or std::nullopt if not found or if
     * the key maps to a Value.
     */
    [[nodiscard]] std::optional<std::shared_ptr<std::any>> GetShared(
        std::string const& key) const;

    /**
     * Checks if a key exists in the series data.
     * @param key The key to check.
     * @return True if the key exists, false otherwise.
     */
    [[nodiscard]] bool Has(std::string const& key) const;

    /**
     * Returns all keys in the series data.
     *
     * Lifetime: The returned vector contains copies of the keys and
     * can be stored safely.
     *
     * @return Vector of all keys.
     */
    [[nodiscard]] std::vector<std::string> Keys() const;

   private:
    friend class EvaluationSeriesDataBuilder;

    struct DataEntry {
        std::optional<Value> value;
        std::optional<std::shared_ptr<std::any>> shared;
    };

    explicit EvaluationSeriesData(std::map<std::string, DataEntry> data);

    std::map<std::string, DataEntry> data_;
};

/**
 * Builder for creating evaluation series data.
 * Allows hook stages to add data to be passed to subsequent stages.
 */
class EvaluationSeriesDataBuilder {
   public:
    /**
     * Creates a new builder from existing data.
     * @param data The existing data to copy.
     */
    explicit EvaluationSeriesDataBuilder(EvaluationSeriesData const& data);

    /**
     * Creates a new empty builder.
     */
    EvaluationSeriesDataBuilder();

    /**
     * Sets a Value in the series data.
     * @param key The key to set.
     * @param value The value to associate with the key.
     * @return Reference to this builder for chaining.
     */
    EvaluationSeriesDataBuilder& Set(std::string key, Value value);

    /**
     * Sets a shared_ptr to any type in the series data.
     *
     * Use this for storing arbitrary objects like OpenTelemetry spans
     * that need to be passed from beforeEvaluation to afterEvaluation.
     *
     * Example:
     * @code
     * auto span = std::make_shared<std::any>(MySpan{});
     * builder.SetShared("span", span);
     * @endcode
     *
     * @param key The key to set.
     * @param value The shared_ptr to associate with the key.
     * @return Reference to this builder for chaining.
     */
    EvaluationSeriesDataBuilder& SetShared(std::string key,
                                           std::shared_ptr<std::any> value);

    /**
     * Builds the immutable series data.
     * @return The built data.
     */
    [[nodiscard]] EvaluationSeriesData Build() const;

   private:
    std::map<std::string, EvaluationSeriesData::DataEntry> data_;
};

/**
 * Context for evaluation series stages.
 * Provides readonly information about the evaluation being performed.
 *
 * Lifetime: Objects of this type are valid only during
 * the execution of a hook stage. Do not store references, pointers, or
 * string_views from this context. If you need any data beyond the stage
 * execution, copy it to owned types (e.g., std::string, Value).
 */
class EvaluationSeriesContext {
   public:
    /**
     * Constructs an evaluation series context.
     * @param flag_key The flag key being evaluated.
     * @param context The context against which the flag is being evaluated.
     * @param default_value The default value for the evaluation.
     * @param method The method being executed.
     * @param hook_context Additional context data provided by the caller.
     * @param environment_id Optional environment ID.
     */
    EvaluationSeriesContext(std::string flag_key,
                            Context const& context,
                            Value default_value,
                            std::string method,
                            HookContext const& hook_context,
                            std::optional<std::string> environment_id);

    /**
     * Returns the flag key being evaluated.
     *
     * Lifetime: The returned string_view is valid only during the
     * execution of the current hook stage. If you need the flag key
     * beyond this call, copy it to a std::string.
     *
     * @return The flag key as a string_view (immutable).
     */
    [[nodiscard]] std::string_view FlagKey() const;

    /**
     * Returns the context against which the flag is being evaluated.
     *
     * Lifetime: The returned Context reference is valid only during
     * the execution of the current hook stage. If you need context
     * data beyond this call, copy the necessary fields.
     *
     * @return The evaluation context.
     */
    [[nodiscard]] Context const& EvaluationContext() const;

    /**
     * Returns the default value provided to the variation method.
     *
     * Lifetime: The returned Value reference is valid only during
     * the execution of the current hook stage. If you need the value
     * beyond this call, make a copy.
     *
     * @return Reference to the default value.
     */
    [[nodiscard]] Value const& DefaultValue() const;

    /**
     * Returns the method being executed.
     * Examples: "BoolVariation", "StringVariationDetail"
     *
     * Lifetime: The returned string_view is valid only during the
     * execution of the current hook stage. If you need the method name
     * beyond this call, copy it to a std::string.
     *
     * @return The method name as a string_view (immutable).
     */
    [[nodiscard]] std::string_view Method() const;

    /**
     * Returns the environment ID if available.
     * Only available once initialization has completed.
     *
     * Lifetime: If present, the returned string_view is valid only during
     * the execution of the current hook stage. If you need the environment
     * ID beyond this call, copy it to a std::string.
     *
     * @return The environment ID as optional string_view, or std::nullopt
     * if not available.
     */
    [[nodiscard]] std::optional<std::string_view> EnvironmentId() const;

    /**
     * Returns the hook context provided by the caller.
     *
     * This contains arbitrary data that the caller wants to pass through
     * to hooks, such as OpenTelemetry span parents.
     *
     * Lifetime: The returned reference is valid only during the execution
     * of the current hook stage.
     *
     * @return Reference to the hook context.
     */
    [[nodiscard]] HookContext const& HookCtx() const;

   private:
    std::string flag_key_;
    Context const& context_;
    Value default_value_;
    std::string method_;
    HookContext const& hook_context_;
    std::optional<std::string> environment_id_;
};

/**
 * Context for track series handlers.
 * Provides readonly information about the track call being performed.
 *
 * Lifetime: Objects of this type are valid only during
 * the execution of a hook stage. Do not store references, pointers, or
 * string_views from this context. If you need any data beyond the stage
 * execution, copy it to owned types (e.g., std::string, Value).
 */
class TrackSeriesContext {
   public:
    /**
     * Constructs a track series context.
     * @param context The context associated with the track call.
     * @param key The event key.
     * @param metric_value Optional metric value.
     * @param data Optional reference to application-specified data.
     * @param hook_context Additional context data provided by the caller.
     * @param environment_id Optional environment ID.
     */
    TrackSeriesContext(Context const& context,
                       std::string key,
                       std::optional<double> metric_value,
                       std::optional<std::reference_wrapper<Value const>> data,
                       HookContext const& hook_context,
                       std::optional<std::string> environment_id);

    /**
     * Returns the context associated with the track call.
     *
     * Lifetime: The returned Context reference is valid only during
     * the execution of the current hook stage. If you need context
     * data beyond this call, copy the necessary fields.
     *
     * @return The context.
     */
    [[nodiscard]] Context const& TrackContext() const;

    /**
     * Returns the key associated with the track call.
     *
     * Lifetime: The returned string_view is valid only during the
     * execution of the current hook stage. If you need the key
     * beyond this call, copy it to a std::string.
     *
     * @return The event key as a string_view (immutable).
     */
    [[nodiscard]] std::string_view Key() const;

    /**
     * Returns the optional metric value associated with the track call.
     * @return The metric value, or std::nullopt if not provided.
     */
    [[nodiscard]] std::optional<double> MetricValue() const;

    /**
     * Returns the application-specified data associated with the track call.
     *
     * Lifetime: The returned reference (if present) is valid only during
     * the execution of the current hook stage. If you need the value
     * beyond this call, make a copy.
     *
     * @return Reference to the data, or std::nullopt if not provided.
     */
    [[nodiscard]] std::optional<std::reference_wrapper<Value const>> Data() const;

    /**
     * Returns the environment ID if available.
     * Only available once initialization has completed.
     *
     * Lifetime: If present, the returned string_view is valid only during
     * the execution of the current hook stage. If you need the environment
     * ID beyond this call, copy it to a std::string.
     *
     * @return The environment ID as optional string_view, or std::nullopt
     * if not available.
     */
    [[nodiscard]] std::optional<std::string_view> EnvironmentId() const;

    /**
     * Returns the hook context provided by the caller.
     *
     * This contains arbitrary data that the caller wants to pass through
     * to hooks, such as OpenTelemetry span parents.
     *
     * Lifetime: The returned reference is valid only during the execution
     * of the current hook stage.
     *
     * @return Reference to the hook context.
     */
    [[nodiscard]] HookContext const& HookCtx() const;

   private:
    Context const& context_;
    std::string key_;
    std::optional<double> metric_value_;
    std::optional<std::reference_wrapper<Value const>> data_;
    HookContext const& hook_context_;
    std::optional<std::string> environment_id_;
};

/**
 * Base interface for hook implementations.
 *
 * All stage methods have default implementations that take no action,
 * allowing hook implementations to only override the stages they need.
 *
 * This interface is designed for forward compatibility - new stages can be
 * added without breaking existing hook implementations.
 *
 * IMPORTANT LIFETIME CONSIDERATIONS:
 * - All context objects passed to hook stages are valid only during the
 *   execution of that stage.
 * - Do not store references, pointers, or string_views from context objects.
 * - If you need data beyond the immediate execution of a stage, copy it to
 *   owned types (std::string, Value, etc.).
 * - EvaluationSeriesData should only be used within the stage or returned
 *   to be passed to the next stage.
 */
class Hook {
   public:
    virtual ~Hook() = default;

    /**
     * Returns metadata about this hook.
     * @return Reference to hook metadata.
     */
    [[nodiscard]] virtual HookMetadata const& Metadata() const = 0;

    /**
     * Called before a flag is evaluated.
     *
     * @param series_context Context for this evaluation (valid only during
     * this call).
     * @param data Data from previous stage (empty for first stage).
     * @return Data to pass to the next stage.
     */
    virtual EvaluationSeriesData BeforeEvaluation(
        EvaluationSeriesContext const& series_context,
        EvaluationSeriesData data);

    /**
     * Called after a flag has been evaluated.
     *
     * @param series_context Context for this evaluation (valid only during
     * this call).
     * @param data Data from the before stage.
     * @param detail The evaluation result.
     * @return Data to pass to the next stage (currently unused).
     */
    virtual EvaluationSeriesData AfterEvaluation(
        EvaluationSeriesContext const& series_context,
        EvaluationSeriesData data,
        EvaluationDetail<Value> const& detail);

    /**
     * Called after a custom event has been enqueued via Track.
     *
     * @param series_context Context for this track call (valid only during
     * this call).
     */
    virtual void AfterTrack(TrackSeriesContext const& series_context);

   protected:
    Hook() = default;
};

}  // namespace launchdarkly::server_side::hooks
