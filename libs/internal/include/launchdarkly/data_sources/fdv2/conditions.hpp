#pragma once

#include <launchdarkly/data_sources/fdv2/ifdv2_condition.hpp>

#include <launchdarkly/async/cancellation.hpp>
#include <launchdarkly/async/promise.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace launchdarkly::internal::data_sources {

/**
 * Base class for conditions that fire after a duration elapses on the
 * orchestrator's executor. Owns the result promise, the cancellation handle
 * for the active timer (if any), and the state required to safely arm,
 * cancel, and resolve the timer across threads.
 *
 * Derived classes implement Inform() to translate orchestrator events into
 * arm/cancel actions on the timer. Subclasses also implement GetType() to
 * report whether they are a fallback or recovery condition.
 *
 * Thread-safe: every method may be called from any thread. The timer state is
 * held behind a mutex in a shared State, so a timer callback firing on the
 * executor is safe against a concurrent Close() from a caller's thread.
 */
class TimedCondition : public IFDv2Condition {
   public:
    TimedCondition(boost::asio::any_io_executor executor,
                   std::chrono::milliseconds timeout);

    ~TimedCondition() override;

    [[nodiscard]] async::Future<Type> Execute() override;

    void Close() override;

   protected:
    /**
     * Arms the timer if not already armed. When the timer fires, the
     * condition's future resolves with GetType(). Safe to call concurrently;
     * a no-op if a timer is already armed or the condition is closed.
     */
    void ArmTimer();

    /**
     * Cancels the active timer if armed, leaving the condition unresolved.
     * Safe to call when no timer is armed.
     */
    void CancelTimer();

   private:
    struct State {
        std::mutex mutex;
        // All protected by mutex. timer_cancel is replaced when the timer is
        // re-armed, so the lock covers the replacement and not just the
        // source's own operations.
        bool closed = false;
        async::Promise<Type> promise;
        std::optional<async::CancellationSource> timer_cancel;
    };

    boost::asio::any_io_executor const executor_;
    std::chrono::milliseconds const timeout_;
    std::shared_ptr<State> const state_;
};

/**
 * Fires after the active synchronizer has been continuously interrupted for
 * the configured timeout. Each CHANGE_SET result cancels any pending timer;
 * the next Interrupted status re-arms it.
 *
 * Thread-safe, as TimedCondition is.
 */
class FallbackCondition final : public TimedCondition {
   public:
    FallbackCondition(boost::asio::any_io_executor executor,
                      std::chrono::milliseconds timeout);

    void Inform(SourceSignal signal) override;

    [[nodiscard]] Type GetType() const override { return Type::kFallback; }
};

/**
 * Fires after the active synchronizer has been running for the configured
 * timeout, regardless of result content. The timer is started at
 * construction; Inform() is a no-op.
 *
 * Thread-safe, as TimedCondition is.
 */
class RecoveryCondition final : public TimedCondition {
   public:
    RecoveryCondition(boost::asio::any_io_executor executor,
                      std::chrono::milliseconds timeout);

    void Inform(SourceSignal signal) override;

    [[nodiscard]] Type GetType() const override { return Type::kRecovery; }
};

/**
 * Builds fresh FallbackCondition instances on demand.
 *
 * Thread-safe: Build() and GetType() may be called from any thread, and
 * hold no state beyond the executor and timeout given at construction.
 */
class FallbackConditionFactory final : public IFDv2ConditionFactory {
   public:
    FallbackConditionFactory(boost::asio::any_io_executor executor,
                             std::chrono::milliseconds timeout);

    [[nodiscard]] std::unique_ptr<IFDv2Condition> Build() override;

    [[nodiscard]] IFDv2Condition::Type GetType() const override;

   private:
    boost::asio::any_io_executor const executor_;
    std::chrono::milliseconds const timeout_;
};

/**
 * Builds fresh RecoveryCondition instances on demand.
 *
 * Thread-safe: Build() and GetType() may be called from any thread, and
 * hold no state beyond the executor and timeout given at construction.
 */
class RecoveryConditionFactory final : public IFDv2ConditionFactory {
   public:
    RecoveryConditionFactory(boost::asio::any_io_executor executor,
                             std::chrono::milliseconds timeout);

    [[nodiscard]] std::unique_ptr<IFDv2Condition> Build() override;

    [[nodiscard]] IFDv2Condition::Type GetType() const override;

   private:
    boost::asio::any_io_executor const executor_;
    std::chrono::milliseconds const timeout_;
};

/**
 * Aggregates a set of conditions into a single Future that resolves with the
 * type of the first condition to fire. Inform() and Close() forward to every
 * underlying condition. If constructed with no conditions, GetFuture()
 * returns a Future that never resolves (until Close).
 *
 * Thread-safe: GetFuture, Inform, and Close may be called from any thread.
 */
class Conditions final {
   public:
    explicit Conditions(
        std::vector<std::unique_ptr<IFDv2Condition>> conditions);

    ~Conditions();

    Conditions(Conditions const&) = delete;
    Conditions(Conditions&&) = delete;
    Conditions& operator=(Conditions const&) = delete;
    Conditions& operator=(Conditions&&) = delete;

    /**
     * Returns a fresh Future that resolves with the type of the first
     * condition to fire. The caller must cancel the source corresponding to
     * `token` once the result is no longer needed, so that the per-call
     * Promise (and its registered continuations) can be released.
     */
    [[nodiscard]] async::Future<IFDv2Condition::Type> GetFuture(
        async::CancellationToken token);

    void Inform(SourceSignal signal);

    void Close();

   private:
    struct PendingEntry {
        std::int64_t id;
        async::Promise<IFDv2Condition::Type> promise;
        std::unique_ptr<async::CancellationCallback> cancel_cb;
    };

    struct State {
        std::mutex mutex;
        // All protected by mutex.
        std::int64_t next_id = 0;
        std::vector<PendingEntry> pending;
        std::optional<IFDv2Condition::Type> aggregate_result;
    };

    std::vector<std::unique_ptr<IFDv2Condition>> conditions_;
    std::shared_ptr<State> const state_;
};

}  // namespace launchdarkly::internal::data_sources
