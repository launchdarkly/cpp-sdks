#include "polling_synchronizer.hpp"
#include "fdv2_polling_impl.hpp"

#include <launchdarkly/network/http_requester.hpp>

#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/post.hpp>

#include <algorithm>
#include <future>
#include <memory>

namespace launchdarkly::server_side::data_systems {

static char const* const kIdentity = "FDv2 polling synchronizer";

// Minimum polling interval to prevent accidentally hammering the service.
static constexpr std::chrono::seconds kMinPollInterval{30};

using data_interfaces::FDv2SourceResult;

FDv2PollingSynchronizer::FDv2PollingSynchronizer(
    boost::asio::any_io_executor const& executor,
    Logger const& logger,
    config::built::ServiceEndpoints const& endpoints,
    config::built::HttpProperties const& http_properties,
    std::optional<std::string> filter_key,
    std::chrono::seconds poll_interval)
    : logger_(logger),
      requester_(executor, http_properties.Tls()),
      endpoints_(endpoints),
      http_properties_(http_properties),
      filter_key_(std::move(filter_key)),
      poll_interval_(std::max(poll_interval, kMinPollInterval)),
      timer_(executor) {
    if (poll_interval < kMinPollInterval) {
        LD_LOG(logger_, LogLevel::kWarn)
            << kIdentity << ": polling interval too frequent, defaulting to "
            << kMinPollInterval.count() << " seconds";
    }
}

network::HttpRequest FDv2PollingSynchronizer::MakeRequest(
    data_model::Selector const& selector) const {
    return MakeFDv2PollRequest(endpoints_, http_properties_, selector,
                               filter_key_);
}

FDv2SourceResult FDv2PollingSynchronizer::Next(
    std::chrono::milliseconds timeout,
    data_model::Selector selector) {
    if (closed_) {
        return FDv2SourceResult{FDv2SourceResult::Shutdown{}};
    }

    auto promise = std::make_shared<std::promise<FDv2SourceResult>>();
    auto future = promise->get_future();

    // Post the actual work to the ASIO thread so that timer_ and
    // cancel_signal_ are only ever accessed from the executor. Calling their
    // methods directly here (on the orchestrator thread) would be a data race,
    // since ASIO I/O objects and cancellation_signal are not thread-safe.
    // future.get() below blocks the orchestrator thread until DoNext/DoPoll
    // sets the promise from the ASIO thread.
    boost::asio::post(
        timer_.get_executor(),
        [this, timeout, selector = std::move(selector), promise]() mutable {
            DoNext(timeout, std::move(selector), std::move(promise));
        });

    return future.get();
}

void FDv2PollingSynchronizer::DoNext(
    std::chrono::milliseconds timeout,
    data_model::Selector selector,
    std::shared_ptr<std::promise<FDv2SourceResult>> promise) {
    if (closed_) {
        promise->set_value(FDv2SourceResult{FDv2SourceResult::Shutdown{}});
        return;
    }

    auto deadline = std::chrono::steady_clock::now() + timeout;

    if (started_) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - last_poll_start_);
        auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(
                         poll_interval_) -
                     elapsed;

        if (delay.count() > 0) {
            auto remaining =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    deadline - std::chrono::steady_clock::now());

            // timeout_timer must outlive this function: the io_context's
            // internal timer heap holds a pointer to the timer's
            // implementation until the async_wait completes, and the
            // destructor cancels any pending async_wait (posting
            // operation_aborted), which would make the parallel_group
            // immediately complete as if the timeout had fired. Capturing
            // the shared_ptr in the callback lambda keeps the timer alive
            // until the group completes.
            auto timeout_timer = std::make_shared<boost::asio::steady_timer>(
                timer_.get_executor());
            timer_.expires_after(delay);
            timeout_timer->expires_after(remaining);

            boost::asio::experimental::make_parallel_group(
                timer_.async_wait(boost::asio::deferred),
                timeout_timer->async_wait(boost::asio::deferred))
                .async_wait(boost::asio::experimental::wait_for_one(),
                            boost::asio::bind_cancellation_slot(
                                cancel_signal_.slot(),
                                [this, deadline, selector = std::move(selector),
                                 promise, timeout_timer](
                                    std::array<std::size_t, 2> order,
                                    boost::system::error_code,
                                    boost::system::error_code) mutable {
                                    if (closed_) {
                                        promise->set_value(FDv2SourceResult{
                                            FDv2SourceResult::Shutdown{}});
                                        return;
                                    }
                                    if (order[0] == 1) {
                                        promise->set_value(FDv2SourceResult{
                                            FDv2SourceResult::Timeout{}});
                                        return;
                                    }
                                    DoPoll(deadline, std::move(selector),
                                           std::move(promise));
                                }));
            return;
        }
    }

    DoPoll(deadline, std::move(selector), std::move(promise));
}

void FDv2PollingSynchronizer::DoPoll(
    std::chrono::time_point<std::chrono::steady_clock> deadline,
    data_model::Selector selector,
    std::shared_ptr<std::promise<FDv2SourceResult>> promise) {
    if (closed_) {
        promise->set_value(FDv2SourceResult{FDv2SourceResult::Shutdown{}});
        return;
    }

    started_ = true;
    last_poll_start_ = std::chrono::steady_clock::now();
    protocol_handler_.Reset();

    auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
        deadline - std::chrono::steady_clock::now());
    timer_.expires_after(remaining);

    boost::asio::experimental::make_parallel_group(
        requester_.Request(MakeRequest(selector), boost::asio::deferred),
        timer_.async_wait(boost::asio::deferred))
        .async_wait(
            boost::asio::experimental::wait_for_one(),
            boost::asio::bind_cancellation_slot(
                cancel_signal_.slot(),
                [this, promise](std::array<std::size_t, 2> order,
                                network::HttpResult poll_result,
                                boost::system::error_code) mutable {
                    if (closed_) {
                        promise->set_value(
                            FDv2SourceResult{FDv2SourceResult::Shutdown{}});
                    } else if (order[0] == 0) {
                        promise->set_value(HandlePollResult(poll_result));
                    } else {
                        promise->set_value(
                            FDv2SourceResult{FDv2SourceResult::Timeout{}});
                    }
                }));
}

void FDv2PollingSynchronizer::Close() {
    closed_ = true;
    // cancel_signal_ is not thread-safe, so emit() must run on the ASIO
    // thread. post() schedules it there rather than calling it directly,
    // which would race with DoNext/DoPoll accessing the signal concurrently.
    boost::asio::post(timer_.get_executor(), [this] {
        cancel_signal_.emit(boost::asio::cancellation_type::all);
    });
}

std::string const& FDv2PollingSynchronizer::Identity() const {
    static std::string const identity = kIdentity;
    return identity;
}

FDv2SourceResult FDv2PollingSynchronizer::HandlePollResult(
    network::HttpResult const& res) {
    return HandleFDv2PollResponse(res, protocol_handler_, logger_, kIdentity);
}

}  // namespace launchdarkly::server_side::data_systems
