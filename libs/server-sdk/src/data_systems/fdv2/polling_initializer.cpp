#include "polling_initializer.hpp"
#include "fdv2_polling_impl.hpp"

#include <launchdarkly/fdv2_protocol_handler.hpp>
#include <launchdarkly/network/http_requester.hpp>

namespace launchdarkly::server_side::data_systems {

static char const* const kIdentity = "FDv2 polling initializer";

using data_interfaces::FDv2SourceResult;

FDv2PollingInitializer::FDv2PollingInitializer(
    boost::asio::any_io_executor const& executor,
    Logger const& logger,
    config::built::ServiceEndpoints const& endpoints,
    config::built::HttpProperties const& http_properties,
    data_model::Selector selector,
    std::optional<std::string> filter_key)
    : request_(MakeFDv2PollRequest(endpoints,
                                   http_properties,
                                   std::move(selector),
                                   std::move(filter_key))),
      requester_(executor, http_properties.Tls()),
      state_(std::make_shared<State>(logger)) {}

FDv2PollingInitializer::~FDv2PollingInitializer() {
    close_promise_.Resolve(std::monostate{});
}

async::Future<FDv2SourceResult> FDv2PollingInitializer::Run() {
    if (!request_.Valid()) {
        LD_LOG(state_->logger, LogLevel::kError)
            << kIdentity << ": invalid polling endpoint URL";
        using ErrorInfo = FDv2SourceResult::ErrorInfo;
        return async::MakeFuture(
            FDv2SourceResult{FDv2SourceResult::TerminalError{
                ErrorInfo{ErrorInfo::ErrorKind::kUnknown, 0,
                          "invalid polling endpoint URL",
                          std::chrono::system_clock::now()},
                false}});
    }

    // Promisify the callback-based HTTP request.
    auto http_promise = std::make_shared<async::Promise<network::HttpResult>>();
    auto http_future = http_promise->GetFuture();
    requester_.Request(request_, [hp = std::move(http_promise)](
                                     network::HttpResult res) mutable {
        hp->Resolve(std::move(res));
    });

    // Race: HTTP result (0) vs close (1).
    return async::WhenAny(http_future, close_promise_.GetFuture())
        .Then(
            [state = state_, http_future = std::move(http_future)](
                std::size_t const& idx) -> FDv2SourceResult {
                if (idx == 1) {
                    return FDv2SourceResult{FDv2SourceResult::Shutdown{}};
                }
                return HandlePollResult(state, *http_future.GetResult());
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
    std::shared_ptr<State> state,
    network::HttpResult const& res) {
    FDv2ProtocolHandler protocol_handler;
    return HandleFDv2PollResponse(res, protocol_handler, state->logger,
                                  kIdentity);
}

}  // namespace launchdarkly::server_side::data_systems
