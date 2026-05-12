#pragma once

#include "fdv2_source_result.hpp"

#include <launchdarkly/async/promise.hpp>

#include <memory>

namespace launchdarkly::server_side::data_interfaces {

/**
 * A condition observes the orchestrator's stream of synchronizer results and
 * fires when criteria for a synchronizer transition are met.
 *
 * Each condition plays one of two roles, identified by Type():
 *   - kFallback: when fired, the orchestrator stops the active synchronizer
 *     and starts the next-preferred one.
 *   - kRecovery: when fired, the orchestrator stops the active fallback
 *     synchronizer and returns to the most-preferred synchronizer.
 *
 * Conditions are stateful: the orchestrator pushes results into a condition
 * via Inform() so the condition can update its internal state (typically a
 * timer). When the condition's criteria are satisfied, the future returned
 * by Execute() resolves with the condition's Type.
 *
 * Conditions are single-use: once fired, they are not re-armed. The
 * orchestrator builds a fresh condition for each synchronizer activation via
 * an IFDv2ConditionFactory.
 *
 * Close() cancels any pending internal work (e.g., a timer) and resolves the
 * future with kCancelled.
 *
 * Implementations must be thread-safe: Execute, Inform, Close, and GetType
 * may be called from any thread.
 */
class IFDv2Condition {
   public:
    enum class Type {
        /** Stop the active synchronizer and start the next-preferred one. */
        kFallback,
        /** Return to the most-preferred synchronizer. */
        kRecovery,
        /** The condition was Close()d before firing; orchestrator ignores. */
        kCancelled,
    };

    /**
     * Returns a Future that resolves with the condition's Type once the
     * condition's criteria are satisfied. May be called multiple times; each
     * call returns a Future referring to the same underlying state.
     */
    [[nodiscard]] virtual async::Future<Type> Execute() = 0;

    /**
     * Pushes a synchronizer result into the condition so it can update any
     * internal state (e.g., arm or cancel a timer).
     */
    virtual void Inform(FDv2SourceResult const& result) = 0;

    /**
     * Cancels any pending internal work and resolves the future returned by
     * Execute() with kCancelled if it has not already resolved. Idempotent.
     */
    virtual void Close() = 0;

    /**
     * Returns the condition's role in the orchestrator.
     */
    [[nodiscard]] virtual Type GetType() const = 0;

    virtual ~IFDv2Condition() = default;
    IFDv2Condition(IFDv2Condition const&) = delete;
    IFDv2Condition(IFDv2Condition&&) = delete;
    IFDv2Condition& operator=(IFDv2Condition const&) = delete;
    IFDv2Condition& operator=(IFDv2Condition&&) = delete;

   protected:
    IFDv2Condition() = default;
};

/**
 * Builds new IFDv2Condition instances on demand. Each call to Build() produces
 * a fresh condition with no prior state.
 *
 * Implementations must be thread-safe: Build and GetType may be called from
 * any thread.
 */
class IFDv2ConditionFactory {
   public:
    [[nodiscard]] virtual std::unique_ptr<IFDv2Condition> Build() = 0;

    /**
     * Returns the type of conditions this factory builds.
     */
    [[nodiscard]] virtual IFDv2Condition::Type GetType() const = 0;

    virtual ~IFDv2ConditionFactory() = default;
    IFDv2ConditionFactory(IFDv2ConditionFactory const&) = delete;
    IFDv2ConditionFactory(IFDv2ConditionFactory&&) = delete;
    IFDv2ConditionFactory& operator=(IFDv2ConditionFactory const&) = delete;
    IFDv2ConditionFactory& operator=(IFDv2ConditionFactory&&) = delete;

   protected:
    IFDv2ConditionFactory() = default;
};

}  // namespace launchdarkly::server_side::data_interfaces
