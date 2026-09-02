#include "polling_synchronizer.hpp"
#include "fdv2_polling_impl.hpp"

#include <launchdarkly/async/timer.hpp>
#include <launchdarkly/config/shared/defaults.hpp>
#include <launchdarkly/fdv2_protocol_handler.hpp>

#include <algorithm>
#include <utility>

namespace launchdarkly::client_side::data_sources {

static char const* const kIdentity = "FDv2 polling synchronizer";

// Floor on the polling interval, to prevent accidentally hammering the
// service. The SDK has one such floor, which FDv1 polling uses too.
static std::chrono::seconds MinPollInterval() {
    return config::shared::Defaults<config::shared::ClientSDK>::PollingConfig()
        .min_polling_interval;
}

FDv2PollingSynchronizer::State::State(
    Logger const& logger,
    boost::asio::any_io_executor const& executor,
    FDv2RequestConfig const& request_config,
    std::chrono::seconds poll_interval,
    std::optional<std::chrono::steady_clock::time_point> last_poll)
    : logger_(logger),
      poll_interval_(std::max(poll_interval, MinPollInterval())),
      request_config_(request_config),
      requester_(executor, request_config.http_properties.Tls()),
      executor_(executor),
      last_poll_start_(last_poll) {}

async::Future<network::HttpResult> FDv2PollingSynchronizer::State::Request(
    data_model::Selector const& selector) const {
    auto request = MakeFDv2PollRequest(request_config_, selector);

    // The promise must be in a shared_ptr because Requester requires
    // copy-constructible callbacks.
    auto promise = std::make_shared<async::Promise<network::HttpResult>>();
    auto future = promise->GetFuture();
    requester_.Request(request, [promise = std::move(promise)](
                                    network::HttpResult const& res) mutable {
        promise->Resolve(res);
    });
    return future;
}

FDv2SourceResult FDv2PollingSynchronizer::State::HandlePollResult(
    network::HttpResult const& res) const {
    FDv2ProtocolHandler protocol_handler;
    return HandleFDv2PollResponse(res, &protocol_handler, logger_, kIdentity);
}

async::Future<bool> FDv2PollingSynchronizer::State::AwaitNextPoll(
    async::CancellationToken token) {
    std::lock_guard lock(mutex_);

    if (!last_poll_start_) {
        return async::MakeFuture(true);
    }
    auto const elapsed = std::chrono::steady_clock::now() - *last_poll_start_;
    if (elapsed >= poll_interval_) {
        return async::MakeFuture(true);
    }
    return async::Delay(executor_, poll_interval_ - elapsed, std::move(token));
}

void FDv2PollingSynchronizer::State::RecordPollStarted() {
    std::lock_guard lock(mutex_);

    last_poll_start_ = std::chrono::steady_clock::now();
}

FDv2PollingSynchronizer::FDv2PollingSynchronizer(
    boost::asio::any_io_executor const& executor,
    Logger const& logger,
    FDv2RequestConfig const& request_config,
    std::chrono::seconds poll_interval,
    std::optional<std::chrono::steady_clock::time_point> last_poll)
    : state_(std::make_shared<State>(logger,
                                     executor,
                                     request_config,
                                     poll_interval,
                                     last_poll)) {
    if (poll_interval < MinPollInterval()) {
        LD_LOG(logger, LogLevel::kWarn)
            << kIdentity << ": polling interval too frequent, defaulting to "
            << MinPollInterval().count() << " seconds";
    }
}

FDv2PollingSynchronizer::~FDv2PollingSynchronizer() {
    Close();
}

async::Future<FDv2SourceResult> FDv2PollingSynchronizer::Next(
    data_model::Selector selector) {
    return DoNext(state_, close_promise_.GetFuture(), std::move(selector));
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
    data_model::Selector selector) {
    if (closed.IsFinished()) {
        return async::MakeFuture(
            FDv2SourceResult{FDv2SourceResult::Shutdown{}});
    }

    async::CancellationSource cancel;
    auto delay_future = state->AwaitNextPoll(cancel.GetToken());

    return async::WhenAny(closed, std::move(delay_future))
        .Then(
            [state = std::move(state), closed = std::move(closed),
             selector = std::move(selector),
             cancel = std::move(cancel)](std::size_t const& idx) mutable
            -> async::Future<FDv2SourceResult> {
                cancel.Cancel();
                if (idx == 0) {
                    return async::MakeFuture(
                        FDv2SourceResult{FDv2SourceResult::Shutdown{}});
                }
                return DoPoll(std::move(state), std::move(closed), selector);
            },
            async::kInlineExecutor);
}

/* static */ async::Future<FDv2SourceResult> FDv2PollingSynchronizer::DoPoll(
    std::shared_ptr<State> state,
    async::Future<std::monostate> closed,
    data_model::Selector const& selector) {
    if (closed.IsFinished()) {
        return async::MakeFuture(
            FDv2SourceResult{FDv2SourceResult::Shutdown{}});
    }

    state->RecordPollStarted();

    auto http_future = state->Request(selector);

    return async::WhenAny(std::move(closed), http_future)
        .Then(
            [state = std::move(state), http_future = std::move(http_future)](
                std::size_t const& idx) mutable -> FDv2SourceResult {
                if (idx == 0) {
                    return FDv2SourceResult{FDv2SourceResult::Shutdown{}};
                }
                return state->HandlePollResult(*http_future.GetResult());
            },
            async::kInlineExecutor);
}

}  // namespace launchdarkly::client_side::data_sources
