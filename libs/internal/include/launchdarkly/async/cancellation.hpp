#pragma once

#include <launchdarkly/async/promise.hpp>

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

// This file implements a cancellation primitive modelled after C++20's
// std::stop_source / std::stop_token / std::stop_callback design: a source
// owns the ability to trigger cancellation, lightweight tokens (derived from
// the source) can be freely passed around, and CancellationCallback provides
// RAII registration of a callback tied to a token.

namespace launchdarkly::async {

// CancellationState is the shared state between a CancellationSource and all
// CancellationTokens and CancellationCallbacks derived from it. This is an
// internal class; use CancellationSource, CancellationToken, and
// CancellationCallback instead.
class CancellationState {
   public:
    using CallbackId = std::size_t;

    // Sentinel returned by Register() when the state was already cancelled;
    // the callback is invoked immediately in that case. Deregister() is a
    // no-op for this value.
    static constexpr CallbackId kAlreadyCancelled = 0;

    CancellationState() = default;
    ~CancellationState() = default;
    CancellationState(CancellationState const&) = delete;
    CancellationState& operator=(CancellationState const&) = delete;
    CancellationState(CancellationState&&) = delete;
    CancellationState& operator=(CancellationState&&) = delete;

    // Registers a callback and returns its ID. If Cancel() has already been
    // called, invokes cb immediately (outside the lock) and returns
    // kAlreadyCancelled.
    CallbackId Register(Continuation<void()> cb) {
        std::unique_lock lock(mutex_);
        if (cancelled_) {
            lock.unlock();
            cb();
            return kAlreadyCancelled;
        }
        CallbackId id = next_id_++;
        callbacks_.emplace(id, std::move(cb));
        return id;
    }

    // Deregisters the callback with the given ID. If the callback is currently
    // executing on another thread, blocks until execution completes. This
    // mirrors the synchronization guarantee of C++20's stop_callback
    // destructor: after Deregister returns, the callback is guaranteed to have
    // either never run or fully completed. No-op if id is kAlreadyCancelled or
    // the callback has already run.
    void Deregister(CallbackId id) {
        if (id == kAlreadyCancelled) {
            return;
        }

        std::unique_lock lock(mutex_);

        // The callback is still pending: remove it before it can run.
        if (callbacks_.erase(id)) {
            return;
        }

        // The callback has already run, or was never registered.
        if (executing_id_ != id) {
            return;
        }

        // The callback is executing on this thread (re-entrant call from
        // within the callback itself): return without waiting to avoid
        // deadlock.
        if (executing_thread_ == std::this_thread::get_id()) {
            return;
        }

        // The callback is executing on another thread. Wait for it to
        // finish. executing_id_ is set while the state lock is held — before
        // unlocking for invocation — so there is no window where the callback
        // is running but executing_id_ is not yet set.
        executing_done_.wait(lock, [this, id] { return executing_id_ != id; });
    }

    // Invokes all registered callbacks in registration order, then clears the
    // pending list. Callbacks are executed one at a time with the lock
    // released during each invocation to prevent deadlocks. No-op if called
    // more than once.
    void Cancel() {
        std::unique_lock lock(mutex_);
        if (cancelled_) {
            return;
        }
        cancelled_ = true;

        while (!callbacks_.empty()) {
            // Extract the next entry while still holding the lock, then set
            // executing_id_ before releasing. This ensures Deregister can
            // never observe a window where the callback is running but
            // executing_id_ is unset.
            auto node = callbacks_.extract(callbacks_.begin());
            executing_id_ = node.key();
            executing_thread_ = std::this_thread::get_id();

            lock.unlock();
            node.mapped()();
            lock.lock();

            executing_id_ = kAlreadyCancelled;
            lock.unlock();
            executing_done_.notify_all();
            lock.lock();
        }
    }

    // Returns true if Cancel() has been called.
    bool IsCancelled() const {
        std::lock_guard lock(mutex_);
        return cancelled_;
    }

   private:
    mutable std::mutex mutex_;
    bool cancelled_ = false;
    CallbackId next_id_ = 1;  // Real IDs start at 1; 0 is kAlreadyCancelled.
    std::map<CallbackId, Continuation<void()>> callbacks_;

    // Tracks which callback (if any) is currently being invoked by Cancel(),
    // and on which thread, to support the blocking destructor in Deregister.
    CallbackId executing_id_ = kAlreadyCancelled;
    std::thread::id executing_thread_;
    std::condition_variable executing_done_;
};

class CancellationToken;

// CancellationSource is the write end of a cancellation pair: call Cancel()
// to signal all operations holding tokens derived from this source.
//
// CancellationSource is copyable; copies share the same underlying
// CancellationState, matching the behaviour of C++20's stop_source.
class CancellationSource {
   public:
    CancellationSource() : state_(std::make_shared<CancellationState>()) {}

    ~CancellationSource() = default;
    CancellationSource(CancellationSource const&) = default;
    CancellationSource& operator=(CancellationSource const&) = default;
    CancellationSource(CancellationSource&&) = default;
    CancellationSource& operator=(CancellationSource&&) = default;

    // Invokes all registered callbacks in registration order. No-op if called
    // more than once.
    void Cancel() { state_->Cancel(); }

    // Returns true if Cancel() has been called.
    bool IsCancelled() const { return state_->IsCancelled(); }

    // Returns a token referring to this source's cancellation state. The
    // token may be freely copied and passed to any number of
    // CancellationCallbacks.
    CancellationToken GetToken() const;

   private:
    std::shared_ptr<CancellationState> state_;
};

// CancellationToken is the read end of a cancellation pair. Tokens are
// obtained from CancellationSource::GetToken() and passed to
// CancellationCallback constructors to register callbacks.
//
// A default-constructed token has no associated state: any
// CancellationCallback constructed from it is never invoked.
//
// CancellationToken is cheap to copy; all copies share the same underlying
// CancellationState.
class CancellationToken {
   public:
    CancellationToken() = default;

    explicit CancellationToken(std::shared_ptr<CancellationState> state)
        : state_(std::move(state)) {}

    // Returns true if the associated source has been cancelled, or false if
    // there is no associated source.
    bool IsCancelled() const { return state_ && state_->IsCancelled(); }

   private:
    std::shared_ptr<CancellationState> state_;

    friend class CancellationCallback;
};

inline CancellationToken CancellationSource::GetToken() const {
    return CancellationToken(state_);
}

// CancellationCallback registers a callback to be invoked when the associated
// CancellationSource is cancelled. The callback is invoked on whichever thread
// calls CancellationSource::Cancel().
//
// The design follows C++20's std::stop_callback:
//
//   - Constructing a CancellationCallback registers the callback. If the
//     source was already cancelled, the callback is invoked immediately in the
//     constructor.
//   - Destroying a CancellationCallback deregisters the callback. If the
//     callback is currently executing on another thread, the destructor blocks
//     until execution completes, preventing use-after-free of anything captured
//     by the callback.
//   - CancellationCallback is non-copyable and non-movable, matching C++20's
//     stop_callback.
class CancellationCallback {
   public:
    CancellationCallback(CancellationToken token, Continuation<void()> cb)
        : state_(token.state_),
          id_(state_ ? state_->Register(std::move(cb))
                     : CancellationState::kAlreadyCancelled) {}

    ~CancellationCallback() {
        if (state_) {
            state_->Deregister(id_);
        }
    }

    CancellationCallback(CancellationCallback const&) = delete;
    CancellationCallback& operator=(CancellationCallback const&) = delete;
    CancellationCallback(CancellationCallback&&) = delete;
    CancellationCallback& operator=(CancellationCallback&&) = delete;

   private:
    std::shared_ptr<CancellationState> state_;
    CancellationState::CallbackId id_;
};

}  // namespace launchdarkly::async
