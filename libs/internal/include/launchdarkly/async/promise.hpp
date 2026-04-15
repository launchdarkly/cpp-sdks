#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace launchdarkly::async {

// Continuation is a move-only type-erased callable, effectively a polyfill
// for C++23's std::move_only_function. It exists because C++17's std::function
// requires all captured variables to be copy-constructible, which prevents
// storing lambdas that capture move-only types.
//
// Continuation is used to represent units of work passed to executors. An
// executor is a callable with signature void(Continuation<void()>) that
// schedules the work to run somewhere — for example, on an ASIO io_context:
//
//   auto executor = [&ioc](Continuation<void()> work) {
//       boost::asio::post(ioc, std::move(work));
//   };
//
// The primary template is declared but not defined; only the partial
// specialization below (which splits Sig into R and Args...) is usable.
// This lets callers write Continuation<void()> instead of Continuation<void>.
template <typename Sig>
class Continuation;

template <typename R, typename... Args>
class Continuation<R(Args...)> {
    // Base and Impl form a classic type-erasure pair. Base is a non-template
    // abstract interface stored via unique_ptr, giving a stable type regardless
    // of F. Impl<F> is the concrete template subclass that holds and calls F.
    struct Base {
        virtual R call(Args...) = 0;
        virtual ~Base() = default;
    };

    template <typename F>
    struct Impl : Base {
        F f;
        Impl(F f) : f(std::move(f)) {}
        R call(Args... args) override { return f(std::forward<Args>(args)...); }
    };

    std::unique_ptr<Base> impl_;

   public:
    // Constructs a Continuation from any callable F. F may be a lambda,
    // function pointer, or other callable; it need not be copy-constructible.
    // F&& is a forwarding reference: accepts any callable by move or copy,
    // then moves it into Impl<F> so Continuation itself owns the callable.
    template <typename F>
    Continuation(F&& f)
        : impl_(std::make_unique<Impl<std::decay_t<F>>>(std::forward<F>(f))) {}
    Continuation(Continuation&&) = default;
    Continuation& operator=(Continuation&&) = default;

    // Invokes the stored callable with the given arguments.
    // Returns whatever the callable returns.
    R operator()(Args... args) const {
        return impl_->call(std::forward<Args>(args)...);
    }
};

template <typename T>
class Promise;

template <typename T>
class Future;

// Type trait to detect whether a type is a Future<T>. The primary template
// defaults to false; the partial specialization matches Future<T> specifically.
template <typename T>
struct is_future : std::false_type {};

template <typename T>
struct is_future<Future<T>> : std::true_type {};

// Type trait to extract T from Future<T>. Only the partial specialization is
// defined, so using future_value on a non-Future type is a compile error.
template <typename T>
struct future_value;

template <typename T>
struct future_value<Future<T>> {
    using type = T;
};

// PromiseInternal holds the shared state between a Promise and its associated
// Futures: the result value, the mutex protecting it, and the list of
// continuations waiting to run when the result is set.
//
// This is an internal class and not intended to be used directly. Promise and
// Future each hold a shared_ptr<PromiseInternal>, which lets multiple Future
// copies all refer to the same underlying state, and lets Promise and Future
// have independent lifetimes while the shared state remains alive as long as
// either end holds it.
template <typename T>
class PromiseInternal {
   public:
    PromiseInternal() = default;
    ~PromiseInternal() = default;
    PromiseInternal(PromiseInternal const&) = delete;
    PromiseInternal& operator=(PromiseInternal const&) = delete;
    PromiseInternal(PromiseInternal&&) = delete;
    PromiseInternal& operator=(PromiseInternal&&) = delete;

    // Sets the result and schedules all registered continuations via their
    // executors. Returns true if the result was set, or false if it was already
    // set by a previous call to Resolve.
    bool Resolve(T result) {
        std::vector<Continuation<void(T const&)>> to_call;
        {
            std::lock_guard lock(mutex_);
            if (result_.has_value()) {
                return false;
            }

            // Move result into storage if possible; otherwise copy.
            if constexpr (std::is_move_assignable_v<T>) {
                result_ = std::move(result);
            } else {
                result_ = result;
            }
            to_call = std::move(continuations_);
        }

        // Call continuations outside the lock so that continuations which
        // re-enter this future (e.g. via GetResult or Then) don't deadlock.
        for (auto& continuation : to_call) {
            // It's safe to access result_ outside the lock here, because it
            // can't be changed again.
            continuation(*result_);
        }

        return true;
    }

    // Returns true if Resolve has been called.
    bool IsFinished() const {
        std::lock_guard lock(mutex_);
        return result_.has_value();
    }

    // Returns a copy of the result, if resolved.
    std::optional<T> GetResult() const {
        std::lock_guard lock(mutex_);
        return result_;
    }

    // Then where the continuation returns R directly, yielding Future<R>.
    template <typename F,
              // Deduce R from what F returns when called with T const&.
              typename R = std::invoke_result_t<F, T const&>,
              // Disable when R is a Future so the flattening overload wins
              // instead.
              typename = std::enable_if_t<!is_future<R>::value>>
    Future<R> Then(F&& continuation,
                   std::function<void(Continuation<void()>)> executor) {
        Promise<R> newPromise;
        Future<R> newFuture = newPromise.GetFuture();

        std::optional<T> already_resolved;
        {
            std::lock_guard lock(mutex_);

            if (result_.has_value()) {
                already_resolved = result_;
            } else {
                continuations_.push_back(
                    [newPromise = std::move(newPromise),
                     continuation = std::move(continuation),
                     executor](T const& result) mutable {
                        executor(Continuation<void()>(
                            [newPromise = std::move(newPromise),
                             continuation = std::move(continuation),
                             result]() mutable {
                                newPromise.Resolve(continuation(result));
                            }));
                    });
                return newFuture;
            }
        }

        // Already resolved: call executor outside the lock so that
        // continuations which re-enter this future don't deadlock.
        executor(Continuation<void()>(
            [newPromise = std::move(newPromise),
             continuation = std::move(continuation),
             result = std::move(already_resolved)]() mutable {
                newPromise.Resolve(continuation(*result));
            }));
        return newFuture;
    }

    // Then where the continuation returns Future<T2> (i.e. R = Future<T2>),
    // yielding a flattened Future<T2> that resolves when the inner future does.
    template <typename F,
              // Deduce R from what F returns when called with T const&.
              typename R = std::invoke_result_t<F, T const&>,
              // Unwrap Future<T2> -> T2 so the return type is Future<T2>, not
              // Future<Future<T2>>.
              typename T2 = typename future_value<R>::type,
              // Only enabled when R is a Future; otherwise the direct overload
              // wins.
              typename = std::enable_if_t<is_future<R>::value>>
    Future<T2> Then(F&& continuation,
                    std::function<void(Continuation<void()>)> executor) {
        Promise<T2> outerPromise;
        Future<T2> outerFuture = outerPromise.GetFuture();

        auto do_work = [outerPromise = std::move(outerPromise),
                        continuation = std::move(continuation),
                        executor](T const& val) mutable {
            Future<T2> innerFuture = continuation(val);
            innerFuture.Then(
                [outerPromise = std::move(outerPromise)](
                    T2 const& inner_val) mutable -> std::monostate {
                    outerPromise.Resolve(inner_val);
                    return {};
                },
                executor);
        };

        std::optional<T> already_resolved;
        {
            std::lock_guard lock(mutex_);
            if (result_.has_value()) {
                already_resolved = result_;
            } else {
                continuations_.push_back([do_work = std::move(do_work),
                                          executor](T const& result) mutable {
                    executor(Continuation<void()>(
                        [do_work = std::move(do_work), result]() mutable {
                            do_work(result);
                        }));
                });
                return outerFuture;
            }
        }

        // Already resolved: call executor outside the lock so that
        // continuations which re-enter this future don't deadlock.
        executor(Continuation<void()>(
            [do_work = std::move(do_work),
             result = std::move(already_resolved)]() mutable {
                do_work(*result);
            }));
        return outerFuture;
    }

   private:
    mutable std::mutex mutex_;
    std::optional<T> result_{};
    std::vector<Continuation<void(T const&)>> continuations_;
};

// Promise is the write end of a one-shot async value, similar to std::promise.
// Create a Promise<T>, hand its Future to a consumer via GetFuture(), then
// call Resolve() exactly once to deliver the value.
//
// Promise is move-only: it cannot be copied, but it can be moved. This
// prevents accidentally resolving the same promise from two places.
//
// Using tl::expected<V, E> as T is the recommended way to represent
// operations that may fail:
//
//   Promise<tl::expected<int, std::string>> promise;
//   Future<tl::expected<int, std::string>> future = promise.GetFuture();
//   // ... hand future to a consumer, then later:
//   promise.Resolve(42);                            // success
//   promise.Resolve(tl::unexpected("timed out"));  // failure
template <typename T>
class Promise {
   public:
    Promise() : internal_(std::make_shared<PromiseInternal<T>>()) {}
    ~Promise() = default;
    Promise(Promise const&) = delete;
    Promise& operator=(Promise const&) = delete;
    Promise(Promise&&) = default;
    Promise& operator=(Promise&&) = default;

    // Sets the result to the given value and schedules any continuations that
    // were registered via Future::Then. Returns true if the result was set, or
    // false if Resolve was already called.
    bool Resolve(T result) {
        if constexpr (std::is_move_constructible_v<T>) {
            return internal_->Resolve(std::move(result));
        } else {
            return internal_->Resolve(result);
        }
    }

    // Returns a Future that will resolve when this Promise is resolved.
    // May be called multiple times; each call returns a Future referring to
    // the same underlying state.
    Future<T> GetFuture() { return Future(internal_); }

   private:
    std::shared_ptr<PromiseInternal<T>> internal_;
};

// Future is the read end of a one-shot async value, similar to std::future,
// but with support for chaining via Then.
//
// A Future is obtained from Promise::GetFuture(). Multiple copies of a Future
// may exist and all refer to the same underlying result. When the associated
// Promise is resolved, all continuations registered via Then are scheduled.
//
// Unlike std::future, Future does not support blocking on the result directly.
// Instead, use Then to attach work that runs once the value is available.
//
// Example using tl::expected<V, E> to represent a fallible async operation:
//
//   boost::asio::io_context ioc;
//   auto executor = [&ioc](Continuation<void()> work) {
//       boost::asio::post(ioc, std::move(work));
//   };
//
//   Future<tl::expected<float, std::string>> result = future.Then(
//       [](tl::expected<int, std::string> const& val)
//               -> tl::expected<float, std::string> {
//           if (!val) return tl::unexpected(val.error());
//           return *val * 1.5f;
//       },
//       executor);
template <typename T>
class Future {
   public:
    Future(std::shared_ptr<PromiseInternal<T>> internal)
        : internal_(std::move(internal)) {}
    ~Future() = default;
    Future(Future const&) = default;
    Future& operator=(Future const&) = default;
    Future(Future&&) = default;
    Future& operator=(Future&&) = default;

    // Returns true if the associated Promise has been resolved.
    bool IsFinished() const { return internal_->IsFinished(); }

    // Returns a copy of the result, if resolved.
    std::optional<T> GetResult() const { return internal_->GetResult(); }

    // Blocks the calling thread until the future resolves or the timeout
    // expires. Returns a copy of the result, if resolved within the timeout.
    template <typename Rep, typename Period>
    std::optional<T> WaitForResult(std::chrono::duration<Rep, Period> timeout) {
        struct State {
            std::mutex mutex;
            std::condition_variable cv;
            bool ready = false;
        };
        auto state = std::make_shared<State>();

        // The continuation ignores T entirely. The executor signals the cv
        // when called, since being called means the original future has
        // resolved.
        Then([](T const&) { return std::monostate{}; },
             [state](Continuation<void()> work) {
                 {
                     std::lock_guard<std::mutex> lock(state->mutex);
                     state->ready = true;
                 }
                 state->cv.notify_one();
                 work();
             });

        std::unique_lock<std::mutex> lock(state->mutex);
        state->cv.wait_for(lock, timeout, [&state] { return state->ready; });

        return GetResult();
    }

    // Registers a continuation to run when this Future resolves, returning a
    // new Future<R> that resolves to the continuation's return value.
    //
    // Parameters:
    //   continuation - Called with the resolved T const& when this Future
    //                  resolves. Must return a value of type R (not a Future).
    //   executor     - Called with the work to schedule when this Future
    //                  resolves. Controls where and when the continuation runs.
    //
    // Returns a Future<R> that resolves to whatever the continuation returns.
    template <typename F,
              // Deduce R from what F returns when called with T const&.
              typename R = std::invoke_result_t<F, T const&>,
              // Disable when R is a Future so the flattening overload wins
              // instead.
              typename = std::enable_if_t<!is_future<R>::value>>
    Future<R> Then(F&& continuation,
                   std::function<void(Continuation<void()>)> executor) {
        return internal_->Then(std::forward<F>(continuation),
                               std::move(executor));
    }

    // Registers a continuation to run when this Future resolves, where the
    // continuation itself returns a Future<T2>. Returns a flattened Future<T2>
    // that resolves when the inner future does, avoiding Future<Future<T2>>.
    // Use this overload to chain async operations that themselves return a
    // Future, avoiding a nested Future<Future<T2>>:
    //
    //   Future<tl::expected<Data, Err>> result = future.Then(
    //       [](tl::expected<Key, Err> const& key) {
    //           return fetch(key);  // fetch returns Future<expected<Data,
    //           Err>>
    //       },
    //       executor);
    //
    // Parameters:
    //   continuation - Called with the resolved T const& when this Future
    //                  resolves. Must return a Future<T2>.
    //   executor     - Called with the work to schedule when this Future
    //                  resolves, and again when the inner Future resolves.
    //
    // Returns a Future<T2> that resolves when the inner Future<T2> resolves.
    template <typename F,
              // Deduce R from what F returns when called with T const&.
              typename R = std::invoke_result_t<F, T const&>,
              // Unwrap Future<T2> -> T2 so the return type is Future<T2>, not
              // Future<Future<T2>>.
              typename T2 = typename future_value<R>::type,
              // Only enabled when R is a Future; otherwise the direct overload
              // wins.
              typename = std::enable_if_t<is_future<R>::value>>
    Future<T2> Then(F&& continuation,
                    std::function<void(Continuation<void()>)> executor) {
        return internal_->Then(std::forward<F>(continuation),
                               std::move(executor));
    }

   private:
    std::shared_ptr<PromiseInternal<T>> internal_;
};

// WhenAll takes a variadic list of Futures (each with potentially different
// value types) and returns a Future<std::monostate> that resolves once all
// of the input futures have resolved. The result carries no value; callers
// who need the individual results can read them from their original futures
// after WhenAll resolves.
//
// If called with no arguments, the returned future is already resolved.
//
// Example:
//
//   Future<int> f1 = ...;
//   Future<std::string> f2 = ...;
//   WhenAll(f1, f2).Then(
//       [&](std::monostate const&) {
//           // f1 and f2 are both finished here.
//           use(f1.GetResult().value(), f2.GetResult().value());
//           return std::monostate{};
//       },
//       executor);
template <typename... Ts>
Future<std::monostate> WhenAll(Future<Ts>... futures) {
    Promise<std::monostate> promise;
    Future<std::monostate> result = promise.GetFuture();

    if constexpr (sizeof...(Ts) == 0) {
        promise.Resolve(std::monostate{});
        return result;
    }

    auto shared_promise =
        std::make_shared<Promise<std::monostate>>(std::move(promise));
    auto count = std::make_shared<std::atomic<std::size_t>>(sizeof...(Ts));

    auto attach = [&](auto future) {
        future.Then(
            [shared_promise, count](auto const&) -> std::monostate {
                if (count->fetch_sub(1) == 1) {
                    shared_promise->Resolve(std::monostate{});
                }
                return std::monostate{};
            },
            [](Continuation<void()> f) { f(); });
    };

    (attach(futures), ...);

    return result;
}

// WhenAny takes a variadic list of Futures (each with potentially different
// value types) and returns a Future<std::size_t> that resolves with the
// 0-based index of whichever input future resolves first. The caller can use
// the index to identify the winning future and read its result directly.
//
// If called with no arguments, the returned future never resolves.
//
// Example:
//
//   Future<int> f0 = ...;
//   Future<std::string> f1 = ...;
//   WhenAny(f0, f1).Then(
//       [&](std::size_t const& index) {
//           if (index == 0) use(f0.GetResult().value());
//           else            use(f1.GetResult().value());
//           return std::monostate{};
//       },
//       executor);
template <typename... Ts>
Future<std::size_t> WhenAny(Future<Ts>... futures) {
    Promise<std::size_t> promise;
    Future<std::size_t> result = promise.GetFuture();

    auto shared_promise =
        std::make_shared<Promise<std::size_t>>(std::move(promise));

    std::size_t index = 0;
    auto attach = [&](auto future) {
        std::size_t i = index++;
        future.Then(
            [shared_promise, i](auto const&) -> std::monostate {
                shared_promise->Resolve(i);
                return std::monostate{};
            },
            [](Continuation<void()> f) { f(); });
    };

    (attach(futures), ...);

    return result;
}

}  // namespace launchdarkly::async
