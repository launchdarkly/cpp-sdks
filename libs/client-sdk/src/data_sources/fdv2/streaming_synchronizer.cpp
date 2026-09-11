#include "streaming_synchronizer.hpp"
#include "fdv2_changeset_translation.hpp"
#include "fdv2_polling_impl.hpp"
#include "fdv2_response_headers.hpp"

#include <launchdarkly/encoding/base_64.hpp>

#include <boost/json.hpp>
#include <boost/url/parse.hpp>

#include <utility>

namespace launchdarkly::client_side::data_sources {

static char const* const kIdentity = "FDv2 streaming synchronizer";

static char const* const kPingEvent = "ping";

// Read-idle timeout for the long-lived stream, larger than the service
// heartbeat so a live connection is not declared dead.
static constexpr std::chrono::minutes kDeadConnectionInterval{5};

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
    Logger const& logger,
    boost::asio::any_io_executor const& executor,
    FDv2RequestConfig const& stream_config,
    FDv2RequestConfig const& poll_config,
    std::chrono::milliseconds initial_reconnect_delay)
    : logger_(logger),
      stream_config_(stream_config),
      poll_config_(poll_config),
      initial_reconnect_delay_(initial_reconnect_delay),
      executor_(executor),
      requester_(executor, poll_config.http_properties.Tls()) {}

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

    bool const post =
        stream_config_.transport == FDv2ContextTransport::kPostBody;

    auto parsed = boost::urls::parse_uri(stream_config_.base_url);
    if (!parsed) {
        // A bad endpoint URL is a configuration error that won't fix itself,
        // so started_ stays true and this synchronizer does not reconnect.
        LD_LOG(logger_, LogLevel::kError)
            << kIdentity << ": could not parse streaming endpoint URL";
        Notify(FDv2SourceResult{FDv2SourceResult::TerminalError{
            MakeError(ErrorKind::kNetworkError, 0,
                      "could not parse streaming endpoint URL")}});
        return;
    }

    boost::urls::url url = parsed.value();

    // A trailing '/' on the base URL appears as an empty final segment.
    // Remove it so the pushed segments do not produce a double slash.
    auto segments = url.segments();
    if (!segments.empty() && segments.back().empty()) {
        segments.pop_back();
    }
    segments.push_back("sdk");
    segments.push_back("stream");
    segments.push_back("eval");
    if (!post) {
        segments.push_back(
            encoding::Base64UrlEncode(stream_config_.serialized_context));
    }
    if (stream_config_.with_reasons) {
        url.params().set("withReasons", "true");
    }

    // The basis parameter is added by the on-connect hook instead, so that
    // each reconnection uses the freshest selector.
    {
        std::lock_guard lock(mutex_);
        base_url_ = url;
    }

    auto builder = sse::Builder(executor_, std::string(url.buffer()));

    builder.method(post ? boost::beast::http::verb::post
                        : boost::beast::http::verb::get);
    if (post) {
        builder.header("content-type", "application/json");
        builder.body(stream_config_.serialized_context);
    }
    builder.read_timeout(kDeadConnectionInterval);
    builder.write_timeout(stream_config_.http_properties.WriteTimeout());
    builder.connect_timeout(stream_config_.http_properties.ConnectTimeout());
    builder.initial_reconnect_delay(initial_reconnect_delay_);

    for (auto const& [key, value] :
         stream_config_.http_properties.BaseHeaders()) {
        builder.header(key, value);
    }
    if (stream_config_.http_properties.Tls().PeerVerifyMode() ==
        config::shared::built::TlsOptions::VerifyMode::kVerifyNone) {
        builder.skip_verify_peer(true);
    }
    if (auto ca_file = stream_config_.http_properties.Tls().CustomCAFile()) {
        builder.custom_ca_file(*ca_file);
    }
    if (auto proxy_url = stream_config_.http_properties.Proxy().Url()) {
        builder.proxy(*proxy_url);
    }

    std::weak_ptr<State> weak = self;
    builder.on_connect([weak](HttpRequest* req) {
        if (auto s = weak.lock()) {
            s->OnConnect(req);
        }
    });
    builder.on_response([weak](HttpResponseHeader const& headers) {
        if (auto s = weak.lock()) {
            s->OnResponse(headers);
        }
    });
    builder.receiver([weak](sse::Event const& event) {
        if (auto s = weak.lock()) {
            s->OnEvent(event);
        }
    });
    builder.logger([weak](std::string msg) {
        if (auto s = weak.lock()) {
            LD_LOG(s->logger_, LogLevel::kDebug) << "sse-client: " << msg;
        }
    });
    builder.errors([weak](sse::Error error) {
        if (auto s = weak.lock()) {
            s->OnError(error);
        }
    });

    auto client = builder.build();
    if (!client) {
        LD_LOG(logger_, LogLevel::kError)
            << kIdentity << ": could not build SSE client";
        Notify(FDv2SourceResult{FDv2SourceResult::TerminalError{MakeError(
            ErrorKind::kNetworkError, 0, "could not build SSE client")}});
        return;
    }

    // If Close() ran while we were building, drop the client and stop.
    std::lock_guard lock(mutex_);
    if (closed_) {
        return;
    }
    sse_client_ = client;
    client->async_connect();
}

void FDv2StreamingSynchronizer::State::OnConnect(HttpRequest* req) {
    std::lock_guard lock(mutex_);
    // base_url_ is guaranteed populated. EnsureStarted publishes it before
    // calling async_connect, which is what eventually triggers this hook.
    boost::urls::url url = *base_url_;
    if (latest_selector_.value) {
        url.params().set("basis", latest_selector_.value->state);
    }
    req->target(url.encoded_target());
}

void FDv2StreamingSynchronizer::State::OnResponse(
    HttpResponseHeader const& headers) {
    auto read = ReadFDv2ResponseHeaders(headers);

    std::lock_guard lock(mutex_);
    latest_fdv1_fallback_ = std::move(read.fdv1_fallback);
    if (read.environment_id) {
        latest_environment_id_ = std::move(read.environment_id);
    }
}

void FDv2StreamingSynchronizer::State::PollForPing(
    std::shared_ptr<State> self) {
    {
        std::lock_guard lock(mutex_);
        if (closed_ || ping_poll_in_flight_) {
            return;
        }
        ping_poll_in_flight_ = true;
    }

    LD_LOG(logger_, LogLevel::kDebug)
        << kIdentity << ": ping received, polling for the current payload";

    data_model::Selector selector;
    {
        std::lock_guard lock(mutex_);
        selector = latest_selector_;
    }

    auto request = MakeFDv2PollRequest(poll_config_, selector);
    requester_.Request(
        request, [self = std::move(self)](network::HttpResult const& res) {
            FDv2ProtocolHandler handler;
            auto result =
                HandleFDv2PollResponse(res, &handler, self->logger_, kIdentity);
            {
                std::lock_guard lock(self->mutex_);
                self->ping_poll_in_flight_ = false;
            }
            self->Notify(std::move(result));
        });
}

void FDv2StreamingSynchronizer::State::OnEvent(sse::Event const& event) {
    if (event.type() == kPingEvent) {
        PollForPing(shared_from_this());
        return;
    }

    if (!FDv2ProtocolHandler::IsKnownEvent(event.type())) {
        return;
    }

    boost::system::error_code ec;
    auto data = boost::json::parse(event.data(), ec);
    if (ec) {
        protocol_handler_.Reset();
        std::string msg = "could not parse FDv2 streaming event payload";
        LD_LOG(logger_, LogLevel::kError) << kIdentity << ": " << msg;
        Notify(FDv2SourceResult{FDv2SourceResult::Interrupted{
            MakeError(ErrorKind::kInvalidData, 0, std::move(msg))}});
        std::lock_guard lock(mutex_);
        if (sse_client_) {
            sse_client_->async_restart("FDv2 parse error");
        }
        return;
    }

    auto result = protocol_handler_.HandleEvent(event.type(), data);

    std::visit(
        [this](auto const& r) {
            using T = std::decay_t<decltype(r)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                // Accumulating, heartbeat, or unknown event -- nothing to do.
            } else if constexpr (std::is_same_v<T, data_model::FDv2ChangeSet>) {
                auto typed = TranslateChangeSet(r, logger_);
                if (!typed) {
                    std::string msg =
                        "FDv2 streaming changeset could not be translated";
                    LD_LOG(logger_, LogLevel::kError)
                        << kIdentity << ": " << msg;
                    Notify(FDv2SourceResult{
                        FDv2SourceResult::Interrupted{MakeError(
                            ErrorKind::kInvalidData, 0, std::move(msg))}});
                    return;
                }
                Notify(FDv2SourceResult{
                    FDv2SourceResult::ChangeSet{std::move(*typed)}});
            } else if constexpr (std::is_same_v<T, Goodbye>) {
                LD_LOG(logger_, LogLevel::kInfo)
                    << kIdentity
                    << ": Goodbye was received from the LaunchDarkly "
                       "connection with reason: '"
                    << r.reason.value_or("") << "'.";
                FDv2SourceResult goodbye_result{
                    FDv2SourceResult::Goodbye{r.reason}};
                if (r.protocol_fallback_ttl) {
                    goodbye_result.fdv1_fallback =
                        FDv1FallbackDirective::FromServiceTtl(
                            std::chrono::seconds(*r.protocol_fallback_ttl));
                }
                Notify(std::move(goodbye_result));
                // Drop the current connection and reconnect. The protocol
                // handler is reset so the new connection starts in a clean
                // state.
                protocol_handler_.Reset();
                std::lock_guard lock(mutex_);
                if (sse_client_) {
                    sse_client_->async_restart("FDv2 goodbye received");
                }
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
                    Notify(FDv2SourceResult{
                        FDv2SourceResult::Interrupted{MakeError(
                            ErrorKind::kErrorResponse, 0, std::move(msg))}});
                    return;
                }
                LD_LOG(logger_, LogLevel::kError)
                    << kIdentity << ": " << r.message;
                Notify(FDv2SourceResult{FDv2SourceResult::Interrupted{
                    MakeError(ErrorKind::kInvalidData, 0, r.message)}});
                std::lock_guard lock(mutex_);
                if (sse_client_) {
                    sse_client_->async_restart("FDv2 protocol error");
                }
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
            MakeError(ErrorKind::kNetworkError, 0, std::move(msg))}});
        return;
    }

    LD_LOG(logger_, LogLevel::kError) << kIdentity << ": " << msg;

    if (auto const* client_error =
            std::get_if<sse::errors::UnrecoverableClientError>(&error)) {
        Notify(FDv2SourceResult{FDv2SourceResult::TerminalError{MakeError(
            ErrorKind::kErrorResponse,
            static_cast<ErrorInfo::StatusCodeType>(client_error->status),
            std::move(msg))}});
        return;
    }

    Notify(FDv2SourceResult{FDv2SourceResult::TerminalError{
        MakeError(ErrorKind::kNetworkError, 0, std::move(msg))}});
}

void FDv2StreamingSynchronizer::State::Notify(FDv2SourceResult result) {
    std::optional<async::Promise<FDv2SourceResult>> promise;
    {
        std::lock_guard lock(mutex_);
        // A directive parsed from the stream, such as one on a goodbye
        // message, takes precedence over the most recent response header.
        if (!result.fdv1_fallback) {
            result.fdv1_fallback = latest_fdv1_fallback_;
        }
        if (!result.environment_id) {
            result.environment_id = latest_environment_id_;
        }
        if (pending_promise_) {
            promise = std::move(pending_promise_);
            pending_promise_.reset();
        } else {
            result_queue_.push_back(std::move(result));
            return;
        }
    }
    // Resolve outside the lock. Promise::Resolve may invoke inline
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

FDv2StreamingSynchronizer::FDv2StreamingSynchronizer(
    boost::asio::any_io_executor const& executor,
    Logger const& logger,
    FDv2RequestConfig const& stream_config,
    FDv2RequestConfig const& poll_config,
    std::chrono::milliseconds initial_reconnect_delay)
    : state_(std::make_shared<State>(logger,
                                     executor,
                                     stream_config,
                                     poll_config,
                                     initial_reconnect_delay)) {}

FDv2StreamingSynchronizer::~FDv2StreamingSynchronizer() {
    Close();
}

async::Future<FDv2SourceResult> FDv2StreamingSynchronizer::Next(
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

    return async::WhenAny(closed, result_future)
        .Then(
            [state = state_, result_future](
                std::size_t const& idx) mutable -> FDv2SourceResult {
                if (idx == 0) {
                    state->ClearPendingPromise();
                    return FDv2SourceResult{FDv2SourceResult::Shutdown{}};
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

}  // namespace launchdarkly::client_side::data_sources
