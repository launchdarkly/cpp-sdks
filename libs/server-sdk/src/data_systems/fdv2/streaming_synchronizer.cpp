#include "streaming_synchronizer.hpp"
#include "fdv2_changeset_translation.hpp"

#include <launchdarkly/async/timer.hpp>

#include <boost/json.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url.hpp>

namespace launchdarkly::server_side::data_systems {

static char const* const kIdentity = "FDv2 streaming synchronizer";

// Maximum time between bytes read from the stream before the SSE client
// declares the connection dead and reconnects. Must be greater than the
// streaming service's heartbeat interval. Hardcoded rather than read from
// HttpProperties, whose default ReadTimeout is sized for one-shot HTTP
// requests and would cause spurious disconnects on a long-lived stream.
static constexpr std::chrono::minutes kDeadConnectionInterval{5};

using data_interfaces::FDv2SourceResult;
using ErrorInfo = FDv2SourceResult::ErrorInfo;
using ErrorKind = ErrorInfo::ErrorKind;

static ErrorInfo MakeError(ErrorKind kind,
                           ErrorInfo::StatusCodeType status,
                           std::string message) {
    return ErrorInfo{kind, status, std::move(message),
                     std::chrono::system_clock::now()};
}

template <class>
inline constexpr bool always_false_v = false;

FDv2StreamingSynchronizer::State::State(
    Logger logger,
    boost::asio::any_io_executor const& executor,
    config::built::ServiceEndpoints const& endpoints,
    config::built::HttpProperties const& http_properties,
    std::optional<std::string> filter_key,
    std::chrono::milliseconds initial_reconnect_delay)
    : logger_(std::move(logger)),
      endpoints_(endpoints),
      http_properties_(http_properties),
      filter_key_(std::move(filter_key)),
      initial_reconnect_delay_(initial_reconnect_delay),
      executor_(executor) {}

void FDv2StreamingSynchronizer::State::EnsureStarted(
    data_model::Selector const& selector,
    std::shared_ptr<State> self) {
    {
        std::lock_guard lock(mutex_);
        latest_selector_ = selector;
        if (closed_ || started_) {
            return;
        }
        started_ = true;
    }

    auto parsed = boost::urls::parse_uri(endpoints_.StreamingBaseUrl());
    if (!parsed) {
        // started_ intentionally left true: a bad endpoint URL is a
        // configuration error that won't fix itself. The TerminalError
        // result tells the orchestrator to stop retrying this synchronizer.
        LD_LOG(logger_, LogLevel::kError)
            << kIdentity << ": could not parse streaming endpoint URL";
        Notify(FDv2SourceResult{FDv2SourceResult::TerminalError{
            MakeError(ErrorKind::kNetworkError, 0,
                      "could not parse streaming endpoint URL"),
            false}});
        return;
    }

    boost::urls::url u = parsed.value();

    // Safer way of appending /sdk/stream than string concatenation: avoids
    // double slashes if the base URL has a trailing slash.
    u.segments().push_back("sdk");
    u.segments().push_back("stream");

    // basis and filter are not added here — they are appended per-connect by
    // the on_connect hook (OnConnect), so that each (re)connection uses the
    // freshest selector.
    {
        std::lock_guard lock(mutex_);
        base_url_ = u;
    }

    auto client_builder = sse::Builder(executor_, std::string(u.buffer()));

    client_builder.method(boost::beast::http::verb::get);
    client_builder.read_timeout(kDeadConnectionInterval);
    client_builder.write_timeout(http_properties_.WriteTimeout());
    client_builder.connect_timeout(http_properties_.ConnectTimeout());
    client_builder.initial_reconnect_delay(initial_reconnect_delay_);

    for (auto const& [key, value] : http_properties_.BaseHeaders()) {
        client_builder.header(key, value);
    }
    if (http_properties_.Tls().PeerVerifyMode() ==
        launchdarkly::config::shared::built::TlsOptions::VerifyMode::
            kVerifyNone) {
        client_builder.skip_verify_peer(true);
    }
    if (auto ca_file = http_properties_.Tls().CustomCAFile()) {
        client_builder.custom_ca_file(*ca_file);
    }
    if (auto proxy_url = http_properties_.Proxy().Url()) {
        client_builder.proxy(*proxy_url);
    }

    std::weak_ptr<State> weak = self;
    client_builder.on_connect([weak](HttpRequest* req) {
        if (auto s = weak.lock()) {
            s->OnConnect(req);
        }
    });
    client_builder.receiver([weak](sse::Event const& event) {
        if (auto s = weak.lock()) {
            s->OnEvent(event);
        }
    });
    client_builder.logger([weak](std::string msg) {
        if (auto s = weak.lock()) {
            LD_LOG(s->logger_, LogLevel::kDebug) << msg;
        }
    });
    client_builder.errors([weak](sse::Error error) {
        if (auto s = weak.lock()) {
            s->OnError(error);
        }
    });

    auto client = client_builder.build();
    if (!client) {
        // started_ intentionally left true: same reasoning as above.
        LD_LOG(logger_, LogLevel::kError)
            << kIdentity << ": could not build SSE client";
        Notify(FDv2SourceResult{FDv2SourceResult::TerminalError{
            MakeError(ErrorKind::kNetworkError, 0,
                      "could not build SSE client"),
            false}});
        return;
    }

    // Publish-and-connect atomically with respect to Shutdown(). If Shutdown
    // ran while we were building, drop the client on the floor — async_connect
    // was never called, so there is nothing to clean up.
    std::lock_guard lock(mutex_);
    if (closed_) {
        return;
    }
    sse_client_ = client;
    client->async_connect();
}

void FDv2StreamingSynchronizer::State::OnConnect(HttpRequest* req) {
    std::lock_guard lock(mutex_);
    // base_url_ is guaranteed populated: EnsureStarted publishes it before
    // calling async_connect, which is what eventually triggers this hook.
    boost::urls::url u = *base_url_;
    if (latest_selector_.value) {
        u.params().set("basis", latest_selector_.value->state);
    }
    if (filter_key_) {
        u.params().set("filter", *filter_key_);
    }
    req->target(u.encoded_target());
}

void FDv2StreamingSynchronizer::State::OnEvent(sse::Event const& event) {
    boost::system::error_code ec;
    auto data = boost::json::parse(event.data(), ec);
    if (ec) {
        protocol_handler_.Reset();
        std::string msg = "could not parse FDv2 streaming event payload";
        LD_LOG(logger_, LogLevel::kError) << kIdentity << ": " << msg;
        Notify(FDv2SourceResult{FDv2SourceResult::Interrupted{
            MakeError(ErrorKind::kInvalidData, 0, std::move(msg)), false}});
        return;
    }

    auto result = protocol_handler_.HandleEvent(event.type(), data);

    std::visit(
        [this](auto const& r) {
            using T = std::decay_t<decltype(r)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                // Accumulating, heartbeat, or unknown event — nothing to do.
            } else if constexpr (std::is_same_v<T, data_model::FDv2ChangeSet>) {
                auto typed = TranslateChangeSet(r, logger_);
                if (!typed) {
                    std::string msg =
                        "FDv2 streaming changeset could not be translated";
                    LD_LOG(logger_, LogLevel::kError)
                        << kIdentity << ": " << msg;
                    Notify(FDv2SourceResult{FDv2SourceResult::Interrupted{
                        MakeError(ErrorKind::kInvalidData, 0, std::move(msg)),
                        false}});
                    return;
                }
                Notify(FDv2SourceResult{
                    FDv2SourceResult::ChangeSet{std::move(*typed), false}});
            } else if constexpr (std::is_same_v<T, Goodbye>) {
                LD_LOG(logger_, LogLevel::kInfo)
                    << kIdentity
                    << ": Goodbye was received from the LaunchDarkly "
                       "connection with reason: '"
                    << r.reason.value_or("") << "'.";
                Notify(FDv2SourceResult{
                    FDv2SourceResult::Goodbye{r.reason, false}});
            } else if constexpr (std::is_same_v<T,
                                                FDv2ProtocolHandler::Error>) {
                if (r.kind == FDv2ProtocolHandler::Error::Kind::kServerError) {
                    auto const& id = r.server_error.value().id;
                    std::string msg =
                        "An issue was encountered receiving updates for "
                        "payload '" +
                        id.value_or("") + "' with reason: '" + r.message +
                        "'. Automatic retry will occur.";
                    LD_LOG(logger_, LogLevel::kInfo)
                        << kIdentity << ": " << msg;
                    Notify(FDv2SourceResult{FDv2SourceResult::Interrupted{
                        MakeError(ErrorKind::kErrorResponse, 0, std::move(msg)),
                        false}});
                    return;
                }
                LD_LOG(logger_, LogLevel::kError)
                    << kIdentity << ": " << r.message;
                Notify(FDv2SourceResult{FDv2SourceResult::Interrupted{
                    MakeError(ErrorKind::kInvalidData, 0, r.message), false}});
            } else {
                static_assert(always_false_v<T>, "non-exhaustive visitor");
            }
        },
        result);
}

void FDv2StreamingSynchronizer::State::OnError(sse::Error const& error) {
    protocol_handler_.Reset();

    std::string msg = sse::ErrorToString(error);

    if (sse::IsRecoverable(error)) {
        LD_LOG(logger_, LogLevel::kWarn) << kIdentity << ": " << msg;
        Notify(FDv2SourceResult{FDv2SourceResult::Interrupted{
            MakeError(ErrorKind::kNetworkError, 0, std::move(msg)), false}});
        return;
    }

    LD_LOG(logger_, LogLevel::kError) << kIdentity << ": " << msg;

    if (auto const* client_error =
            std::get_if<sse::errors::UnrecoverableClientError>(&error)) {
        Notify(FDv2SourceResult{FDv2SourceResult::TerminalError{
            MakeError(
                ErrorKind::kErrorResponse,
                static_cast<ErrorInfo::StatusCodeType>(client_error->status),
                std::move(msg)),
            false}});
        return;
    }

    Notify(FDv2SourceResult{FDv2SourceResult::TerminalError{
        MakeError(ErrorKind::kNetworkError, 0, std::move(msg)), false}});
}

void FDv2StreamingSynchronizer::State::Notify(FDv2SourceResult result) {
    std::optional<async::Promise<FDv2SourceResult>> promise;
    {
        std::lock_guard lock(mutex_);
        if (pending_promise_) {
            promise = std::move(pending_promise_);
            pending_promise_.reset();
        } else {
            result_queue_.push_back(std::move(result));
            return;
        }
    }
    // Resolve outside the lock — Promise::Resolve may invoke inline
    // continuations that could call back into Notify or Next.
    promise->Resolve(std::move(result));
}

async::Future<FDv2SourceResult> FDv2StreamingSynchronizer::State::Next(
    data_model::Selector const& selector,
    std::shared_ptr<State> self) {
    EnsureStarted(selector, std::move(self));

    std::lock_guard lock(mutex_);
    if (!result_queue_.empty()) {
        auto result = std::move(result_queue_.front());
        result_queue_.pop_front();
        return async::MakeFuture(std::move(result));
    }
    return pending_promise_.emplace().GetFuture();
}

void FDv2StreamingSynchronizer::State::ClearPendingPromise() {
    std::lock_guard lock(mutex_);
    pending_promise_.reset();
}

void FDv2StreamingSynchronizer::State::Shutdown() {
    std::shared_ptr<sse::Client> client;
    {
        std::lock_guard lock(mutex_);
        closed_ = true;
        client = std::exchange(sse_client_, nullptr);
    }
    if (client) {
        client->async_shutdown([] {});
    }
}

async::Future<bool> FDv2StreamingSynchronizer::State::Delay(
    std::chrono::milliseconds duration,
    async::CancellationToken token) {
    return async::Delay(executor_, duration, std::move(token));
}

FDv2StreamingSynchronizer::FDv2StreamingSynchronizer(
    boost::asio::any_io_executor const& executor,
    Logger const& logger,
    config::built::ServiceEndpoints const& endpoints,
    config::built::HttpProperties const& http_properties,
    std::optional<std::string> filter_key,
    std::chrono::milliseconds initial_reconnect_delay)
    : state_(std::make_shared<State>(logger,
                                     executor,
                                     endpoints,
                                     http_properties,
                                     std::move(filter_key),
                                     initial_reconnect_delay)) {}

FDv2StreamingSynchronizer::~FDv2StreamingSynchronizer() {
    Close();
}

async::Future<FDv2SourceResult> FDv2StreamingSynchronizer::Next(
    std::chrono::milliseconds timeout,
    data_model::Selector selector) {
    auto closed = close_promise_.GetFuture();
    if (closed.IsFinished()) {
        return async::MakeFuture(
            FDv2SourceResult{FDv2SourceResult::Shutdown{}});
    }

    auto result_future = state_->Next(selector, state_);
    if (result_future.IsFinished()) {
        return result_future;
    }

    async::CancellationSource cancel;
    auto timeout_future = state_->Delay(timeout, cancel.GetToken());

    return async::WhenAny(closed, timeout_future, result_future)
        .Then(
            [state = state_, cancel = std::move(cancel), result_future](
                std::size_t const& idx) mutable -> FDv2SourceResult {
                cancel.Cancel();
                if (idx == 0) {
                    state->ClearPendingPromise();
                    return FDv2SourceResult{FDv2SourceResult::Shutdown{}};
                }
                if (idx == 1) {
                    state->ClearPendingPromise();
                    if (result_future.IsFinished()) {
                        return *result_future.GetResult();
                    }
                    return FDv2SourceResult{FDv2SourceResult::Timeout{}};
                }
                return *result_future.GetResult();
            },
            async::kInlineExecutor);
}

void FDv2StreamingSynchronizer::Close() {
    if (!close_promise_.Resolve(std::monostate{})) {
        return;
    }
    state_->Shutdown();
}

std::string const& FDv2StreamingSynchronizer::Identity() const {
    static std::string const identity = kIdentity;
    return identity;
}

}  // namespace launchdarkly::server_side::data_systems
