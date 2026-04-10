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
template <typename Sig>
class Continuation;

template <typename R, typename... Args>
class Continuation<R(Args...)> {
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

    template <typename F, typename T2 = std::invoke_result_t<F, T const&>>
    Future<T2> Then(F&& continuation,
                    std::function<void(Continuation<void()>)> executor) {
        Promise<T2> newPromise;
        std::lock_guard lock(mutex_);

        Future<T2> newFuture = newPromise.GetFuture();

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
                     continuation = std::move(continuation), result]() mutable {
                        newPromise.Resolve(continuation(**result));
                    }));
            });

        return newFuture;
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

    template <typename F, typename T2 = std::invoke_result_t<F, T const&>>
    Future<T2> Then(F&& continuation,
                    std::function<void(Continuation<void()>)> executor) {
        return internal_->Then(std::forward<F>(continuation),
                               std::move(executor));
    }

   private:
    std::shared_ptr<PromiseInternal<T>> internal_;
};

}  // namespace launchdarkly::async
