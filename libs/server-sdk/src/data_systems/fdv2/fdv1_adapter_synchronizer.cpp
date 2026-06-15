#include "fdv1_adapter_synchronizer.hpp"

#include <utility>

namespace launchdarkly::server_side::data_systems {

using data_interfaces::FDv2SourceResult;
using DataSourceState = DataSourceStatus::DataSourceState;

// ----- State -----

FDv1AdapterSynchronizer::State::State(
    async::Future<std::monostate> closed_future)
    : closed_future_(std::move(closed_future)) {}

async::Future<FDv2SourceResult> FDv1AdapterSynchronizer::State::GetNext() {
    std::lock_guard lock(mutex_);
    if (!result_queue_.empty()) {
        auto result = std::move(result_queue_.front());
        result_queue_.pop_front();
        return async::MakeFuture(std::move(result));
    }
    return pending_promise_.emplace().GetFuture();
}

void FDv1AdapterSynchronizer::State::ResolvePendingAsShutdown() {
    std::optional<async::Promise<FDv2SourceResult>> promise;
    {
        std::lock_guard lock(mutex_);
        if (pending_promise_) {
            promise = std::move(pending_promise_);
            pending_promise_.reset();
        }
    }
    if (promise) {
        promise->Resolve(FDv2SourceResult{FDv2SourceResult::Shutdown{}});
    }
}

void FDv1AdapterSynchronizer::State::Notify(FDv2SourceResult result) {
    std::optional<async::Promise<FDv2SourceResult>> promise;
    {
        std::lock_guard lock(mutex_);
        if (closed_future_.IsFinished()) {
            return;
        }
        if (pending_promise_) {
            promise = std::move(pending_promise_);
            pending_promise_.reset();
        } else {
            result_queue_.push_back(std::move(result));
            return;
        }
    }
    // Resolve outside the lock — Promise::Resolve may invoke inline
    // continuations that could call back into Notify or GetNext.
    promise->Resolve(std::move(result));
}

// ----- ConvertingDestination -----

FDv1AdapterSynchronizer::ConvertingDestination::ConvertingDestination(
    std::weak_ptr<State> state)
    : state_(std::move(state)) {}

void FDv1AdapterSynchronizer::ConvertingDestination::Init(
    data_model::SDKDataSet data_set) {
    auto state = state_.lock();
    if (!state) {
        return;
    }
    data_interfaces::ChangeSetData changes;
    changes.reserve(data_set.flags.size() + data_set.segments.size());
    for (auto& [key, flag] : data_set.flags) {
        changes.push_back({key, std::move(flag)});
    }
    for (auto& [key, segment] : data_set.segments) {
        changes.push_back({key, std::move(segment)});
    }
    state->Notify(FDv2SourceResult{FDv2SourceResult::ChangeSet{
        data_model::ChangeSet<data_interfaces::ChangeSetData>{
            data_model::ChangeSetType::kFull, std::move(changes),
            data_model::Selector{}}}});
}

void FDv1AdapterSynchronizer::ConvertingDestination::Upsert(
    std::string const& key,
    data_model::FlagDescriptor flag) {
    auto state = state_.lock();
    if (!state) {
        return;
    }
    data_interfaces::ChangeSetData changes;
    changes.push_back({key, std::move(flag)});
    state->Notify(FDv2SourceResult{FDv2SourceResult::ChangeSet{
        data_model::ChangeSet<data_interfaces::ChangeSetData>{
            data_model::ChangeSetType::kPartial, std::move(changes),
            data_model::Selector{}}}});
}

void FDv1AdapterSynchronizer::ConvertingDestination::Upsert(
    std::string const& key,
    data_model::SegmentDescriptor segment) {
    auto state = state_.lock();
    if (!state) {
        return;
    }
    data_interfaces::ChangeSetData changes;
    changes.push_back({key, std::move(segment)});
    state->Notify(FDv2SourceResult{FDv2SourceResult::ChangeSet{
        data_model::ChangeSet<data_interfaces::ChangeSetData>{
            data_model::ChangeSetType::kPartial, std::move(changes),
            data_model::Selector{}}}});
}

std::string const& FDv1AdapterSynchronizer::ConvertingDestination::Identity()
    const {
    static std::string const identity = "FDv1 adapter destination";
    return identity;
}

// ----- FDv1AdapterSynchronizer -----

FDv1AdapterSynchronizer::FDv1AdapterSynchronizer(SourceBuilder source_builder)
    : state_(std::make_shared<State>(close_promise_.GetFuture())),
      destination_(std::make_unique<ConvertingDestination>(state_)),
      status_manager_(
          std::make_unique<data_components::DataSourceStatusManager>()),
      status_subscription_(status_manager_->OnDataSourceStatusChange(
          [state = state_](DataSourceStatus status) {
              auto error = status.LastError();
              if (!error) {
                  return;
              }
              switch (status.State()) {
                  case DataSourceState::kInterrupted:
                  case DataSourceState::kInitializing:
                      // Recoverable error.
                      state->Notify(FDv2SourceResult{
                          FDv2SourceResult::Interrupted{*error}});
                      break;
                  case DataSourceState::kOff:
                      // Terminal error.
                      state->Notify(FDv2SourceResult{
                          FDv2SourceResult::TerminalError{*error}});
                      break;
                  case DataSourceState::kValid:
                      // Recovery; no FDv2 result.
                      break;
              }
          })),
      fdv1_source_(source_builder(*status_manager_)) {}

FDv1AdapterSynchronizer::~FDv1AdapterSynchronizer() {
    Close();
}

async::Future<FDv2SourceResult> FDv1AdapterSynchronizer::Next(
    data_model::Selector /*selector*/) {
    auto closed = close_promise_.GetFuture();
    if (closed.IsFinished()) {
        return async::MakeFuture(
            FDv2SourceResult{FDv2SourceResult::Shutdown{}});
    }
    {
        std::lock_guard lock(lifecycle_mutex_);
        if (!started_) {
            started_ = true;
            fdv1_source_->StartAsync(destination_.get(),
                                     /*bootstrap_data=*/nullptr);
        }
    }
    auto result_future = state_->GetNext();
    if (result_future.IsFinished()) {
        return result_future;
    }
    return async::WhenAny(closed, result_future)
        .Then(
            [state = state_, result_future](std::size_t const& idx) mutable
                -> async::Future<FDv2SourceResult> {
                if (idx == 0) {
                    state->ResolvePendingAsShutdown();
                    return async::MakeFuture(
                        FDv2SourceResult{FDv2SourceResult::Shutdown{}});
                }
                return result_future;
            },
            async::kInlineExecutor);
}

void FDv1AdapterSynchronizer::Close() {
    if (!close_promise_.Resolve(std::monostate{})) {
        return;
    }
    std::lock_guard lock(lifecycle_mutex_);
    bool const was_started = started_;
    started_ = true;
    if (was_started) {
        fdv1_source_->ShutdownAsync([] {});
    }
}

std::string const& FDv1AdapterSynchronizer::Identity() const {
    static std::string const identity = "FDv1 fallback adapter";
    return identity;
}

}  // namespace launchdarkly::server_side::data_systems
