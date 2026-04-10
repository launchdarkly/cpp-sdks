#pragma once

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
        Impl(F&& f) : f(std::move(f)) {}
        R call(Args... args) override { return f(std::forward<Args>(args)...); }
    };

    std::unique_ptr<Base> impl_;

   public:
    // F&& is a forwarding reference: accepts any callable by move or copy,
    // then moves it into Impl<F> so Continuation itself owns the callable.
    template <typename F>
    Continuation(F&& f)
        : impl_(std::make_unique<Impl<F>>(std::forward<F>(f))) {}
    Continuation(Continuation&&) = default;
    Continuation& operator=(Continuation&&) = default;

    R operator()(Args... args) {
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

// TODO: Document why things are what they are.
// TODO: Is it okay that the shared_ptr lets copies mutate the result??

template <typename T>
class PromiseInternal {
   public:
    PromiseInternal() = default;
    ~PromiseInternal() = default;
    PromiseInternal(PromiseInternal const&) = delete;
    PromiseInternal(PromiseInternal&&) = delete;

    bool Resolve(T result) {
        std::lock_guard lock(mutex_);
        if (result_->has_value()) {
            return false;
        }

        *result_ = result;

        for (auto& continuation : continuations_) {
            continuation(result_);
        }
        continuations_.clear();

        return true;
    }

    bool IsFinished() const {
        std::lock_guard lock(mutex_);
        return result_->has_value();
    }

    T const& GetResult() const {
        std::lock_guard lock(mutex_);
        return **result_;
    }

    // Then where the continuation returns R directly, yielding Future<R>.
    template <typename F,
              // Deduce R from what F returns when called with T const&.
              typename R = std::invoke_result_t<F, T const&>,
              // Disable when R is a Future so the flattening overload wins instead.
              typename = std::enable_if_t<!is_future<R>::value>>
    Future<R> Then(F&& continuation,
                   std::function<void(Continuation<void()>)> executor) {
        Promise<R> newPromise;
        std::lock_guard lock(mutex_);

        Future<R> newFuture = newPromise.GetFuture();

        if (result_->has_value()) {
            executor(Continuation<void()>(
                [newPromise = std::move(newPromise),
                 continuation = std::move(continuation),
                 result = result_]() mutable {
                    newPromise.Resolve(continuation(**result));
                }));
            return newFuture;
        }

        continuations_.push_back(
            [newPromise = std::move(newPromise),
             continuation = std::move(continuation),
             executor](std::shared_ptr<std::optional<T>> result) mutable {
                executor(Continuation<void()>(
                    [newPromise = std::move(newPromise),
                     continuation = std::move(continuation),
                     result]() mutable {
                        newPromise.Resolve(continuation(**result));
                    }));
            });

        return newFuture;
    }

    // Then where the continuation returns Future<T2> (i.e. R = Future<T2>),
    // yielding a flattened Future<T2> that resolves when the inner future does.
    template <typename F,
              // Deduce R from what F returns when called with T const&.
              typename R = std::invoke_result_t<F, T const&>,
              // Unwrap Future<T2> -> T2 so the return type is Future<T2>, not Future<Future<T2>>.
              typename T2 = typename future_value<R>::type,
              // Only enabled when R is a Future; otherwise the direct overload wins.
              typename = std::enable_if_t<is_future<R>::value>>
    Future<T2> Then(F&& continuation,
                    std::function<void(Continuation<void()>)> executor) {
        Promise<T2> outerPromise;
        std::lock_guard lock(mutex_);

        Future<T2> outerFuture = outerPromise.GetFuture();

        auto do_work = [outerPromise = std::move(outerPromise),
                        continuation = std::move(continuation),
                        executor](T const& val) mutable {
            Future<T2> innerFuture = continuation(val);
            innerFuture.Then(
                [outerPromise = std::move(outerPromise)](
                    T2 const& inner_val) mutable -> T2 {
                    outerPromise.Resolve(inner_val);
                    return inner_val;
                },
                executor);
        };

        if (result_->has_value()) {
            executor(Continuation<void()>(
                [do_work = std::move(do_work), result = result_]() mutable {
                    do_work(**result);
                }));
            return outerFuture;
        }

        continuations_.push_back(
            [do_work = std::move(do_work),
             executor](std::shared_ptr<std::optional<T>> result) mutable {
                executor(Continuation<void()>(
                    [do_work = std::move(do_work), result]() mutable {
                        do_work(**result);
                    }));
            });

        return outerFuture;
    }

   private:
    std::mutex mutex_;
    std::shared_ptr<std::optional<T>> result_{
        new std::optional<T>(std::nullopt)};
    std::vector<Continuation<void(std::shared_ptr<std::optional<T>>)>>
        continuations_;
};

template <typename T>
class Promise {
   public:
    Promise() : internal_(new PromiseInternal<T>()) {}
    ~Promise() = default;
    Promise(Promise const&) = delete;
    Promise(Promise&&) = default;

    bool Resolve(T result) { return internal_->Resolve(result); }

    Future<T> GetFuture() { return Future(internal_); }

   private:
    std::shared_ptr<PromiseInternal<T>> internal_;
};

template <typename T>
class Future {
   public:
    Future(std::shared_ptr<PromiseInternal<T>> internal)
        : internal_(internal) {}
    ~Future() = default;
    Future(Future const&) = default;
    Future(Future&&) = default;

    bool IsFinished() const { return internal_->IsFinished(); }

    T const& GetResult() const { return internal_->GetResult(); }

    // Then where the continuation returns R directly, yielding Future<R>.
    template <typename F,
              // Deduce R from what F returns when called with T const&.
              typename R = std::invoke_result_t<F, T const&>,
              // Disable when R is a Future so the flattening overload wins instead.
              typename = std::enable_if_t<!is_future<R>::value>>
    Future<R> Then(F&& continuation,
                   std::function<void(Continuation<void()>)> executor) {
        return internal_->Then(std::forward<F>(continuation),
                               std::move(executor));
    }

    // Then where the continuation returns Future<T2> (i.e. R = Future<T2>),
    // yielding a flattened Future<T2> that resolves when the inner future does.
    template <typename F,
              // Deduce R from what F returns when called with T const&.
              typename R = std::invoke_result_t<F, T const&>,
              // Unwrap Future<T2> -> T2 so the return type is Future<T2>, not Future<Future<T2>>.
              typename T2 = typename future_value<R>::type,
              // Only enabled when R is a Future; otherwise the direct overload wins.
              typename = std::enable_if_t<is_future<R>::value>>
    Future<T2> Then(F&& continuation,
                    std::function<void(Continuation<void()>)> executor) {
        return internal_->Then(std::forward<F>(continuation),
                               std::move(executor));
    }

   private:
    std::shared_ptr<PromiseInternal<T>> internal_;
};

}  // namespace launchdarkly::async
