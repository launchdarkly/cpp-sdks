#include "conditions.hpp"

#include <launchdarkly/async/timer.hpp>

#include <algorithm>
#include <utility>
#include <variant>

namespace launchdarkly::server_side::data_systems {

using data_interfaces::FDv2SourceResult;
using data_interfaces::IFDv2Condition;

TimedCondition::TimedCondition(boost::asio::any_io_executor executor,
                               std::chrono::milliseconds timeout)
    : executor_(std::move(executor)),
      timeout_(timeout),
      state_(std::make_shared<State>()) {}

TimedCondition::~TimedCondition() {
    Close();
}

async::Future<IFDv2Condition::Type> TimedCondition::Execute() {
    std::lock_guard lock(state_->mutex);
    return state_->promise.GetFuture();
}

void TimedCondition::Close() {
    {
        std::lock_guard lock(state_->mutex);
        if (state_->closed) {
            return;
        }
        state_->closed = true;
        if (state_->timer_cancel) {
            state_->timer_cancel->Cancel();
            state_->timer_cancel.reset();
        }
    }
    state_->promise.Resolve(IFDv2Condition::Type::kCancelled);
}

void TimedCondition::ArmTimer() {
    Type const type = GetType();
    auto state = state_;

    std::lock_guard lock(state->mutex);
    if (state->closed || state->timer_cancel.has_value()) {
        return;
    }
    state->timer_cancel.emplace();
    async::Delay(executor_, timeout_, state->timer_cancel->GetToken())
        .Then(
            [state, type](bool const& fired_normally) -> std::monostate {
                if (fired_normally) {
                    state->promise.Resolve(type);
                }
                return {};
            },
            [](async::Continuation<void()> work) { work(); });
}

void TimedCondition::CancelTimer() {
    std::lock_guard lock(state_->mutex);
    if (state_->timer_cancel) {
        state_->timer_cancel->Cancel();
        state_->timer_cancel.reset();
    }
}

FallbackCondition::FallbackCondition(boost::asio::any_io_executor executor,
                                     std::chrono::milliseconds timeout)
    : TimedCondition(std::move(executor), timeout) {}

void FallbackCondition::Inform(FDv2SourceResult const& result) {
    if (std::get_if<FDv2SourceResult::ChangeSet>(&result.value)) {
        CancelTimer();
    } else if (std::get_if<FDv2SourceResult::Interrupted>(&result.value)) {
        ArmTimer();
    }
}

RecoveryCondition::RecoveryCondition(boost::asio::any_io_executor executor,
                                     std::chrono::milliseconds timeout)
    : TimedCondition(std::move(executor), timeout) {
    ArmTimer();
}

void RecoveryCondition::Inform(FDv2SourceResult const&) {}

FallbackConditionFactory::FallbackConditionFactory(
    boost::asio::any_io_executor executor,
    std::chrono::milliseconds timeout)
    : executor_(std::move(executor)), timeout_(timeout) {}

std::unique_ptr<IFDv2Condition> FallbackConditionFactory::Build() {
    return std::make_unique<FallbackCondition>(executor_, timeout_);
}

IFDv2Condition::Type FallbackConditionFactory::GetType() const {
    return IFDv2Condition::Type::kFallback;
}

RecoveryConditionFactory::RecoveryConditionFactory(
    boost::asio::any_io_executor executor,
    std::chrono::milliseconds timeout)
    : executor_(std::move(executor)), timeout_(timeout) {}

std::unique_ptr<IFDv2Condition> RecoveryConditionFactory::Build() {
    return std::make_unique<RecoveryCondition>(executor_, timeout_);
}

IFDv2Condition::Type RecoveryConditionFactory::GetType() const {
    return IFDv2Condition::Type::kRecovery;
}

namespace {

async::Future<IFDv2Condition::Type> MakeAggregateFuture(
    std::vector<std::unique_ptr<IFDv2Condition>> const& conditions) {
    std::vector<async::Future<IFDv2Condition::Type>> futures;
    futures.reserve(conditions.size());
    for (auto const& condition : conditions) {
        futures.push_back(condition->Execute());
    }
    return async::WhenAny(futures).Then(
        [futures](std::size_t const& idx) -> IFDv2Condition::Type {
            return *futures[idx].GetResult();
        },
        async::kInlineExecutor);
}

}  // namespace

Conditions::Conditions(std::vector<std::unique_ptr<IFDv2Condition>> conditions)
    : conditions_(std::move(conditions)), state_(std::make_shared<State>()) {
    MakeAggregateFuture(conditions_)
        .Then(
            [state =
                 state_](IFDv2Condition::Type const& type) -> std::monostate {
                std::vector<PendingEntry> drained;
                {
                    std::lock_guard lock(state->mutex);
                    state->aggregate_result = type;
                    drained = std::move(state->pending);
                }
                for (auto& entry : drained) {
                    entry.promise.Resolve(type);
                }
                return {};
            },
            async::kInlineExecutor);
}

Conditions::~Conditions() {
    Close();
}

async::Future<IFDv2Condition::Type> Conditions::GetFuture(
    async::CancellationToken token) {
    async::Promise<IFDv2Condition::Type> promise;
    auto future = promise.GetFuture();
    std::int64_t id;

    {
        std::lock_guard lock(state_->mutex);
        if (state_->aggregate_result) {
            promise.Resolve(*state_->aggregate_result);
            return future;
        }
        id = state_->next_id++;
        state_->pending.push_back({id, std::move(promise), nullptr});
    }

    // Construct cb outside the lock: if the token is already cancelled, the
    // callback fires synchronously inside the ctor and needs to acquire
    // state_->mutex.
    auto cb = std::make_unique<async::CancellationCallback>(
        token, [state = state_, id]() {
            std::lock_guard lock(state->mutex);
            state->pending.erase(
                std::remove_if(
                    state->pending.begin(), state->pending.end(),
                    [id](PendingEntry const& e) { return e.id == id; }),
                state->pending.end());
        });

    // Re-find the entry under the lock and attach cb. Between the push above
    // and here, the aggregate may have fired and drained the vector, or cb's
    // callback may have fired synchronously during construction and erased
    // the entry. If gone, drop cb.
    {
        std::lock_guard lock(state_->mutex);
        auto it =
            std::find_if(state_->pending.begin(), state_->pending.end(),
                         [id](PendingEntry const& e) { return e.id == id; });
        if (it != state_->pending.end()) {
            it->cancel_cb = std::move(cb);
        }
    }

    return future;
}

void Conditions::Inform(FDv2SourceResult const& result) {
    for (auto const& condition : conditions_) {
        condition->Inform(result);
    }
}

void Conditions::Close() {
    for (auto const& condition : conditions_) {
        condition->Close();
    }
    // Resolve any promises still pending.
    std::vector<PendingEntry> drained;
    {
        std::lock_guard lock(state_->mutex);
        if (!state_->aggregate_result) {
            state_->aggregate_result = IFDv2Condition::Type::kCancelled;
        }
        drained = std::move(state_->pending);
    }
    for (auto& entry : drained) {
        entry.promise.Resolve(IFDv2Condition::Type::kCancelled);
    }
}

}  // namespace launchdarkly::server_side::data_systems
