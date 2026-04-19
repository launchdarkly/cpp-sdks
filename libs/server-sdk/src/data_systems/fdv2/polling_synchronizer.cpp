#include "polling_synchronizer.hpp"
#include "fdv2_polling_impl.hpp"

#include <launchdarkly/async/timer.hpp>
#include <launchdarkly/network/http_requester.hpp>

#include <algorithm>

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
      executor_(executor),
      requester_(executor, http_properties.Tls()),
      endpoints_(endpoints),
      http_properties_(http_properties),
      filter_key_(std::move(filter_key)),
      poll_interval_(std::max(poll_interval, kMinPollInterval)) {
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

async::Future<FDv2SourceResult> FDv2PollingSynchronizer::Next(
    std::chrono::milliseconds timeout,
    data_model::Selector selector) {
    if (closed_) {
        return async::MakeFuture(
            FDv2SourceResult{FDv2SourceResult::Shutdown{}});
    }

    auto deadline = std::chrono::steady_clock::now() + timeout;

    if (!started_) {
        return DoPoll(deadline, selector);
    }

    auto elapsed = std::chrono::steady_clock::now() - last_poll_start_;
    auto delay = poll_interval_ - elapsed;

    if (delay.count() <= 0) {
        return DoPoll(deadline, selector);
    }

    auto remaining = deadline - std::chrono::steady_clock::now();

    // Use the smaller of the two durations. If the timeout fires
    // first (timeout_first == true), return Timeout; otherwise the
    // inter-poll delay elapsed, so proceed to DoPoll.
    bool timeout_first = remaining <= delay;
    auto effective_delay = timeout_first ? remaining : delay;

    return async::WhenAny(close_promise_.GetFuture(),
                          async::Delay(executor_, effective_delay))
        .Then(
            [this, deadline, selector = std::move(selector),
             timeout_first](std::size_t const& idx) mutable
                -> async::Future<FDv2SourceResult> {
                if (idx == 0 || closed_) {
                    return async::MakeFuture(
                        FDv2SourceResult{FDv2SourceResult::Shutdown{}});
                }
                if (timeout_first) {
                    return async::MakeFuture(
                        FDv2SourceResult{FDv2SourceResult::Timeout{}});
                }
                return DoPoll(deadline, selector);
            },
            async::kInlineExecutor);
}

async::Future<FDv2SourceResult> FDv2PollingSynchronizer::DoPoll(
    std::chrono::time_point<std::chrono::steady_clock> deadline,
    data_model::Selector const& selector) {
    if (closed_) {
        return async::MakeFuture(
            FDv2SourceResult{FDv2SourceResult::Shutdown{}});
    }

    started_ = true;
    last_poll_start_ = std::chrono::steady_clock::now();
    {
        std::lock_guard lock(protocol_handler_mutex_);
        protocol_handler_.Reset();
    }

    auto remaining = deadline - std::chrono::steady_clock::now();

    // Promisify the callback-based HTTP request.
    auto http_promise = std::make_shared<async::Promise<network::HttpResult>>();
    auto http_future = http_promise->GetFuture();
    requester_.Request(MakeRequest(selector),
                       [hp = http_promise](network::HttpResult res) mutable {
                           hp->Resolve(std::move(res));
                       });

    // Race: close (0) vs HTTP result (1) vs timeout (2).
    // The winner unblocks the orchestrator immediately; the losers complete
    // in the background and their Resolve() calls are no-ops.
    return async::WhenAny(close_promise_.GetFuture(), http_future,
                          async::Delay(executor_, remaining))
        .Then(
            [this, http_future](std::size_t const& idx) -> FDv2SourceResult {
                if (idx == 0 || closed_) {
                    return FDv2SourceResult{FDv2SourceResult::Shutdown{}};
                }
                if (idx == 1) {
                    return HandlePollResult(*http_future.GetResult());
                }
                return FDv2SourceResult{FDv2SourceResult::Timeout{}};
            },
            async::kInlineExecutor);
}

void FDv2PollingSynchronizer::Close() {
    closed_ = true;
    close_promise_.Resolve(std::monostate{});
}

std::string const& FDv2PollingSynchronizer::Identity() const {
    static std::string const identity = kIdentity;
    return identity;
}

FDv2SourceResult FDv2PollingSynchronizer::HandlePollResult(
    network::HttpResult const& res) {
    std::lock_guard lock(protocol_handler_mutex_);
    return HandleFDv2PollResponse(res, protocol_handler_, logger_, kIdentity);
}

}  // namespace launchdarkly::server_side::data_systems
