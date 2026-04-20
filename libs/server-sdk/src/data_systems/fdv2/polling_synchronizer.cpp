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

FDv2PollingSynchronizer::State::State(
    Logger logger,
    boost::asio::any_io_executor const& executor,
    std::chrono::seconds poll_interval,
    config::built::ServiceEndpoints const& endpoints,
    config::built::HttpProperties const& http_properties,
    std::optional<std::string> filter_key,
    async::Future<std::monostate> closed)
    : logger_(logger),
      requester_(executor, http_properties.Tls()),
      executor_(executor),
      poll_interval_(std::max(poll_interval, kMinPollInterval)),
      endpoints_(endpoints),
      http_properties_(http_properties),
      filter_key_(std::move(filter_key)) {}

FDv2PollingSynchronizer::FDv2PollingSynchronizer(
    boost::asio::any_io_executor const& executor,
    Logger const& logger,
    config::built::ServiceEndpoints const& endpoints,
    config::built::HttpProperties const& http_properties,
    std::optional<std::string> filter_key,
    std::chrono::seconds poll_interval)
    : state_(std::make_shared<State>(logger,
                                     executor,
                                     poll_interval,
                                     endpoints,
                                     http_properties,
                                     std::move(filter_key),
                                     close_promise_.GetFuture())) {
    if (poll_interval < kMinPollInterval) {
        LD_LOG(logger, LogLevel::kWarn)
            << kIdentity << ": polling interval too frequent, defaulting to "
            << kMinPollInterval.count() << " seconds";
    }
}

async::Future<FDv2SourceResult> FDv2PollingSynchronizer::Next(
    std::chrono::milliseconds timeout,
    data_model::Selector selector) {
    return DoNext(state_, close_promise_.GetFuture(), timeout, selector);
}

void FDv2PollingSynchronizer::Close() {
    close_promise_.Resolve(std::monostate{});
}

std::string const& FDv2PollingSynchronizer::Identity() const {
    static std::string const identity = kIdentity;
    return identity;
}

/* static */ async::Future<FDv2SourceResult> FDv2PollingSynchronizer::DoNext(
    std::shared_ptr<State> state,
    async::Future<std::monostate> closed,
    std::chrono::milliseconds timeout,
    data_model::Selector const& selector) {
    auto now = std::chrono::steady_clock::now();
    auto timeout_deadline = now + timeout;
    auto timeout_future = state->Delay(timeout);

    if (closed.IsFinished()) {
        return async::MakeFuture(
            FDv2SourceResult{FDv2SourceResult::Shutdown{}});
    }

    // Figure out how much to delay before starting.
    auto delay_future = state->CreatePollDelayFuture();

    return async::WhenAny(closed, timeout_future, delay_future)
        .Then(
            [state, closed, timeout_deadline,
             selector = std::move(selector)](std::size_t const& idx) mutable
                -> async::Future<FDv2SourceResult> {
                if (idx == 0) {
                    return async::MakeFuture(
                        FDv2SourceResult{FDv2SourceResult::Shutdown{}});
                }
                if (idx == 1) {
                    return async::MakeFuture(
                        FDv2SourceResult{FDv2SourceResult::Timeout{}});
                }
                return DoPoll(state, closed, timeout_deadline, selector);
            },
            async::kInlineExecutor);
}

async::Future<network::HttpResult> FDv2PollingSynchronizer::State::Request(
    data_model::Selector const& selector) const {
    auto request = MakeFDv2PollRequest(endpoints_, http_properties_, selector,
                                       filter_key_);

    // Promise must be in a shared_ptr because AsioRequester requires callbacks
    // to be copy-constructible (stored in std::function).
    auto promise = std::make_shared<async::Promise<network::HttpResult>>();
    auto future = promise->GetFuture();
    requester_.Request(request, [promise](network::HttpResult res) mutable {
        promise->Resolve(std::move(res));
    });
    return future;
}

/* static */ async::Future<FDv2SourceResult> FDv2PollingSynchronizer::DoPoll(
    std::shared_ptr<State> state,
    async::Future<std::monostate> closed,
    std::chrono::time_point<std::chrono::steady_clock> timeout_deadline,
    data_model::Selector const& selector) {
    if (closed.IsFinished()) {
        return async::MakeFuture(
            FDv2SourceResult{FDv2SourceResult::Shutdown{}});
    }

    state->RecordPollStarted();

    auto now = std::chrono::steady_clock::now();
    auto timeout_future = state->Delay(timeout_deadline - now);
    auto http_future = state->Request(selector);

    return async::WhenAny(closed, timeout_future, http_future)
        .Then(
            [state, http_future](std::size_t const& idx) -> FDv2SourceResult {
                if (idx == 0) {
                    return FDv2SourceResult{FDv2SourceResult::Shutdown{}};
                }
                if (idx == 1) {
                    return FDv2SourceResult{FDv2SourceResult::Timeout{}};
                }
                return state->HandlePollResult(*http_future.GetResult());
            },
            async::kInlineExecutor);
}

FDv2SourceResult FDv2PollingSynchronizer::State::HandlePollResult(
    network::HttpResult const& res) {
    std::lock_guard lock(mutex_);
    FDv2ProtocolHandler protocol_handler;
    return HandleFDv2PollResponse(res, protocol_handler, logger_, kIdentity);
}

async::Future<bool> FDv2PollingSynchronizer::State::CreatePollDelayFuture() {
    std::lock_guard lock(mutex_);
    if (!started_) {
        return async::MakeFuture(true);
    }
    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - last_poll_start_;
    if (elapsed >= poll_interval_) {
        return async::MakeFuture(true);
    }
    return Delay(poll_interval_ - elapsed);
}

void FDv2PollingSynchronizer::State::RecordPollStarted() {
    std::lock_guard lock(mutex_);
    started_ = true;
    last_poll_start_ = std::chrono::steady_clock::now();
}

}  // namespace launchdarkly::server_side::data_systems
