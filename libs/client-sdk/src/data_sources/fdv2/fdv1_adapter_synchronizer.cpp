#include "fdv1_adapter_synchronizer.hpp"

#include <utility>

namespace launchdarkly::client_side::data_sources {

using DataSourceState = DataSourceStatus::DataSourceState;

// ----- State -----

FDv1AdapterSynchronizer::State::State(async::Future<std::monostate> closed)
    : closed_(std::move(closed)) {}

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
        if (closed_.IsFinished()) {
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
    // Resolve outside the lock. Promise::Resolve may invoke inline
    // continuations that could call back into Notify or GetNext.
    promise->Resolve(std::move(result));
}

// ----- ConvertingSink -----

FDv1AdapterSynchronizer::ConvertingSink::ConvertingSink(
    std::weak_ptr<State> state)
    : state_(std::move(state)) {}

void FDv1AdapterSynchronizer::ConvertingSink::Init(
    Context const& /* context */,
    std::unordered_map<std::string, ItemDescriptor> data) {
    auto state = state_.lock();
    if (!state) {
        return;
    }
    FlagChangeSetData changes;
    changes.reserve(data.size());
    for (auto& [key, item] : data) {
        changes.push_back(FlagChange{key, std::move(item)});
    }
    state->Notify(FDv2SourceResult{FDv2SourceResult::ChangeSet{
        FlagChangeSet{data_model::ChangeSetType::kFull, std::move(changes),
                      data_model::Selector{}}}});
}

void FDv1AdapterSynchronizer::ConvertingSink::Upsert(
    Context const& /* context */,
    std::string key,
    ItemDescriptor item) {
    auto state = state_.lock();
    if (!state) {
        return;
    }
    state->Notify(FDv2SourceResult{FDv2SourceResult::ChangeSet{
        FlagChangeSet{data_model::ChangeSetType::kPartial,
                      {FlagChange{std::move(key), std::move(item)}},
                      data_model::Selector{}}}});
}

void FDv1AdapterSynchronizer::ConvertingSink::Apply(
    Context const& /* context */,
    FlagChangeSet change_set,
    bool /* from_cache */) {
    auto state = state_.lock();
    if (!state) {
        return;
    }
    state->Notify(
        FDv2SourceResult{FDv2SourceResult::ChangeSet{std::move(change_set)}});
}

// ----- FDv1AdapterSynchronizer -----

namespace {

// Turns the wrapped source's status into the result the orchestrator acts on.
// A valid status carries no error and needs no result. The changeset that
// accompanied it already reported the recovery.
std::optional<FDv2SourceResult> ResultForStatus(
    DataSourceStatus const& status) {
    auto const error = status.LastError();
    if (!error) {
        return std::nullopt;
    }
    switch (status.State()) {
        case DataSourceState::kInterrupted:
        // An error encountered before the source ever became valid is
        // reported as still initializing, but it is the same recoverable
        // failure.
        case DataSourceState::kInitializing:
            return FDv2SourceResult{FDv2SourceResult::Interrupted{*error}};
        case DataSourceState::kShutdown:
            return FDv2SourceResult{FDv2SourceResult::TerminalError{*error}};
        case DataSourceState::kValid:
        case DataSourceState::kSetOffline:
            return std::nullopt;
    }
    return std::nullopt;
}

}  // namespace

FDv1AdapterSynchronizer::FDv1AdapterSynchronizer(SourceBuilder source_builder)
    : state_(std::make_shared<State>(close_promise_.GetFuture())),
      sink_(std::make_shared<ConvertingSink>(state_)),
      status_manager_(std::make_shared<DataSourceStatusManager>()),
      status_subscription_(status_manager_->OnDataSourceStatusChange(
          [state = state_](DataSourceStatus status) {
              if (auto result = ResultForStatus(status)) {
                  state->Notify(std::move(*result));
              }
          })),
      fdv1_source_(source_builder(sink_.get(), status_manager_.get())) {}

FDv1AdapterSynchronizer::~FDv1AdapterSynchronizer() {
    Close();
}

async::Future<FDv2SourceResult> FDv1AdapterSynchronizer::Next(
    data_model::Selector /* selector */) {
    auto closed = close_promise_.GetFuture();
    if (closed.IsFinished()) {
        return async::MakeFuture(
            FDv2SourceResult{FDv2SourceResult::Shutdown{}});
    }
    {
        std::lock_guard lock(lifecycle_mutex_);
        if (!started_) {
            started_ = true;
            fdv1_source_->Start();
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
        // The sink and status manager are captured so that they outlive any
        // callback the source has already queued.
        fdv1_source_->ShutdownAsync(
            [sink = sink_, status = status_manager_, source = fdv1_source_] {});
    }
}

std::string const& FDv1AdapterSynchronizer::Identity() const {
    static std::string const identity = "FDv1 fallback adapter";
    return identity;
}

}  // namespace launchdarkly::client_side::data_sources
