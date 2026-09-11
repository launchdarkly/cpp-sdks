#include "polling_initializer.hpp"
#include "fdv2_polling_impl.hpp"

#include <launchdarkly/data_model/selector.hpp>
#include <launchdarkly/fdv2_protocol_handler.hpp>

#include <utility>

namespace launchdarkly::client_side::data_sources {

static char const* const kIdentity = "FDv2 polling initializer";

FDv2PollingInitializer::FDv2PollingInitializer(
    boost::asio::any_io_executor const& executor,
    Logger const& logger,
    FDv2RequestConfig const& request_config)
    : logger_(logger),
      request_(MakeFDv2PollRequest(request_config, data_model::Selector{})),
      requester_(executor, request_config.http_properties.Tls()) {}

FDv2PollingInitializer::~FDv2PollingInitializer() {
    close_promise_.Resolve(std::monostate{});
}

async::Future<FDv2SourceResult> FDv2PollingInitializer::Run() {
    if (!request_.Valid()) {
        LD_LOG(logger_, LogLevel::kError)
            << kIdentity << ": invalid polling endpoint URL";
        using ErrorInfo = FDv2SourceResult::ErrorInfo;
        return async::MakeFuture(
            FDv2SourceResult{FDv2SourceResult::TerminalError{
                ErrorInfo{ErrorInfo::ErrorKind::kNetworkError, 0,
                          "invalid polling endpoint URL",
                          std::chrono::system_clock::now()}}});
    }

    // The promise must be in a shared_ptr because Requester requires
    // copy-constructible callbacks.
    auto http_promise = std::make_shared<async::Promise<network::HttpResult>>();
    auto http_future = http_promise->GetFuture();
    requester_.Request(request_, [hp = std::move(http_promise)](
                                     network::HttpResult const& res) mutable {
        hp->Resolve(res);
    });

    // WhenAny reports index 0 for the HTTP result and 1 for close.
    return async::WhenAny(http_future, close_promise_.GetFuture())
        .Then(
            [logger = logger_, http_future = std::move(http_future)](
                std::size_t const& idx) -> FDv2SourceResult {
                if (idx == 1) {
                    return FDv2SourceResult{FDv2SourceResult::Shutdown{}};
                }
                return HandlePollResult(logger, *http_future.GetResult());
            },
            async::kInlineExecutor);
}

void FDv2PollingInitializer::Close() {
    close_promise_.Resolve(std::monostate{});
}

std::string const& FDv2PollingInitializer::Identity() const {
    static std::string const identity = kIdentity;
    return identity;
}

FDv2SourceResult FDv2PollingInitializer::HandlePollResult(
    Logger const& logger,
    network::HttpResult const& res) {
    FDv2ProtocolHandler protocol_handler;
    return HandleFDv2PollResponse(res, &protocol_handler, logger, kIdentity);
}

}  // namespace launchdarkly::client_side::data_sources
