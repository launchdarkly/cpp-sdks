#include "polling_synchronizer.hpp"
#include "fdv2_polling_impl.hpp"

#include <launchdarkly/async/timer.hpp>
#include <launchdarkly/fdv2_protocol_handler.hpp>
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
    std::string polling_base_url,
    config::built::HttpProperties const& http_properties,
    std::optional<std::string> filter_key)
    : logger_(std::move(logger)),
      poll_interval_(std::max(poll_interval, kMinPollInterval)),
      polling_base_url_(std::move(polling_base_url)),
      http_properties_(http_properties),
      filter_key_(std::move(filter_key)),
      requester_(executor, http_properties.Tls()),
      executor_(executor) {}

async::Future<network::HttpResult> FDv2PollingSynchronizer::State::Request(
    data_model::Selector const& selector) const {
    auto request = MakeFDv2PollRequest(polling_base_url_, http_properties_,
                                       selector, filter_key_, logger_);

    // Promise must be in a shared_ptr because Requester requires callbacks
    // to be copy-constructible (stored in std::function).
    auto promise = std::make_shared<async::Promise<network::HttpResult>>();
    auto future = promise->GetFuture();
    requester_.Request(request, [promise = std::move(promise)](
                                    network::HttpResult const& res) mutable {
        promise->Resolve(res);
    });
    return future;
}

FDv2SourceResult FDv2PollingSynchronizer::State::HandlePollResult(
    network::HttpResult const& res) {
    FDv2ProtocolHandler protocol_handler;
    return HandleFDv2PollResponse(res, &protocol_handler, logger_, kIdentity);
}

async::Future<bool> FDv2PollingSynchronizer::State::Delay(
    std::chrono::nanoseconds duration,
    async::CancellationToken token) {
    return async::Delay(executor_, duration, std::move(token));
}

async::Future<bool> FDv2PollingSynchronizer::State::CreatePollDelayFuture(
    async::CancellationToken token) {
    std::lock_guard lock(mutex_);
    if (!last_poll_start_) {
        return async::MakeFuture(true);
    }
    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - *last_poll_start_;
    if (elapsed >= poll_interval_) {
        return async::MakeFuture(true);
    }
    return Delay(poll_interval_ - elapsed, std::move(token));
}

void FDv2PollingSynchronizer::State::RecordPollStarted() {
    std::lock_guard lock(mutex_);
    last_poll_start_ = std::chrono::steady_clock::now();
}

FDv2PollingSynchronizer::FDv2PollingSynchronizer(
    boost::asio::any_io_executor const& executor,
    Logger const& logger,
    std::string polling_base_url,
    config::built::HttpProperties const& http_properties,
    std::optional<std::string> filter_key,
    std::chrono::seconds poll_interval)
    : state_(std::make_shared<State>(logger,
                                     executor,
                                     poll_interval,
                                     std::move(polling_base_url),
                                     http_properties,
                                     std::move(filter_key))) {
    if (poll_interval < kMinPollInterval) {
        LD_LOG(logger, LogLevel::kWarn)
            << kIdentity << ": polling interval too frequent, defaulting to "
            << kMinPollInterval.count() << " seconds";
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
    auto delay_future = state->CreatePollDelayFuture(cancel.GetToken());

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

}  // namespace launchdarkly::server_side::data_systems
