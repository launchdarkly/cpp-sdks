#pragma once

#include "../../data_interfaces/source/ifdv2_condition.hpp"

#include <launchdarkly/async/cancellation.hpp>
#include <launchdarkly/async/promise.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>

namespace launchdarkly::server_side::data_systems {

/**
 * Base class for conditions that fire after a duration elapses on the
 * orchestrator's executor. Owns the result promise, the cancellation handle
 * for the active timer (if any), and the state required to safely arm,
 * cancel, and resolve the timer across threads.
 *
 * Derived classes implement Inform() to translate orchestrator events into
 * arm/cancel actions on the timer. Subclasses also implement GetType() to
 * report whether they are a fallback or recovery condition.
 */
class TimedCondition : public data_interfaces::IFDv2Condition {
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
 */
class FallbackCondition final : public TimedCondition {
   public:
    FallbackCondition(boost::asio::any_io_executor executor,
                      std::chrono::milliseconds timeout);

    void Inform(data_interfaces::FDv2SourceResult const& result) override;

    [[nodiscard]] Type GetType() const override { return Type::kFallback; }
};

/**
 * Fires after the active synchronizer has been running for the configured
 * timeout, regardless of result content. The timer is started at
 * construction; Inform() is a no-op.
 */
class RecoveryCondition final : public TimedCondition {
   public:
    RecoveryCondition(boost::asio::any_io_executor executor,
                      std::chrono::milliseconds timeout);

    void Inform(data_interfaces::FDv2SourceResult const& result) override;

    [[nodiscard]] Type GetType() const override { return Type::kRecovery; }
};

/**
 * Builds fresh FallbackCondition instances on demand.
 */
class FallbackConditionFactory final
    : public data_interfaces::IFDv2ConditionFactory {
   public:
    FallbackConditionFactory(boost::asio::any_io_executor executor,
                             std::chrono::milliseconds timeout);

    [[nodiscard]] std::unique_ptr<data_interfaces::IFDv2Condition> Build()
        override;

    [[nodiscard]] data_interfaces::IFDv2Condition::Type GetType()
        const override;

   private:
    boost::asio::any_io_executor const executor_;
    std::chrono::milliseconds const timeout_;
};

/**
 * Builds fresh RecoveryCondition instances on demand.
 */
class RecoveryConditionFactory final
    : public data_interfaces::IFDv2ConditionFactory {
   public:
    RecoveryConditionFactory(boost::asio::any_io_executor executor,
                             std::chrono::milliseconds timeout);

    [[nodiscard]] std::unique_ptr<data_interfaces::IFDv2Condition> Build()
        override;

    [[nodiscard]] data_interfaces::IFDv2Condition::Type GetType()
        const override;

   private:
    boost::asio::any_io_executor const executor_;
    std::chrono::milliseconds const timeout_;
};

/**
 * Aggregates a set of conditions into a single Future that resolves with the
 * type of the first condition to fire. Inform() and Close() forward to every
 * underlying condition. If constructed with no conditions, GetFuture()
 * returns a Future that never resolves.
 *
 * Thread-safe: GetFuture, Inform, and Close may be called from any thread.
 */
class Conditions final {
   public:
    explicit Conditions(
        std::vector<std::unique_ptr<data_interfaces::IFDv2Condition>>
            conditions);

    ~Conditions();

    Conditions(Conditions const&) = delete;
    Conditions(Conditions&&) = delete;
    Conditions& operator=(Conditions const&) = delete;
    Conditions& operator=(Conditions&&) = delete;

    [[nodiscard]] async::Future<data_interfaces::IFDv2Condition::Type>
    GetFuture() const;

    void Inform(data_interfaces::FDv2SourceResult const& result);

    void Close();

   private:
    std::vector<std::unique_ptr<data_interfaces::IFDv2Condition>> conditions_;
    async::Future<data_interfaces::IFDv2Condition::Type> future_;
};

}  // namespace launchdarkly::server_side::data_systems
