#include "polling_initializer.hpp"
#include "fdv2_polling_impl.hpp"

#include <launchdarkly/network/http_requester.hpp>

#include <memory>

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
    : logger_(logger),
      request_(MakeFDv2PollRequest(endpoints, http_properties, selector,
                                   filter_key)),
      requester_(executor, http_properties.Tls()) {}

FDv2SourceResult FDv2PollingInitializer::Run() {
    if (!request_.Valid()) {
        LD_LOG(logger_, LogLevel::kError)
            << kIdentity << ": invalid polling endpoint URL";
        using ErrorInfo = FDv2SourceResult::ErrorInfo;
        return FDv2SourceResult{FDv2SourceResult::TerminalError{
            ErrorInfo{ErrorInfo::ErrorKind::kUnknown, 0,
                      "invalid polling endpoint URL",
                      std::chrono::system_clock::now()},
            false}};
    }

    auto shared_result = std::make_shared<std::optional<network::HttpResult>>();

    requester_.Request(request_,
                       [this, shared_result](network::HttpResult res) {
                           std::lock_guard guard(mutex_);
                           *shared_result = std::move(res);
                           cv_.notify_one();
                       });

    std::unique_lock lock(mutex_);
    cv_.wait(lock, [&] { return shared_result->has_value() || closed_; });

    if (closed_) {
        return FDv2SourceResult{FDv2SourceResult::Shutdown{}};
    }

    auto http_result = std::move(**shared_result);
    lock.unlock();

    return HandlePollResult(http_result);
}

void FDv2PollingInitializer::Close() {
    std::lock_guard lock(mutex_);
    closed_ = true;
    cv_.notify_one();
}

std::string const& FDv2PollingInitializer::Identity() const {
    static std::string const identity = kIdentity;
    return identity;
}

FDv2SourceResult FDv2PollingInitializer::HandlePollResult(
    network::HttpResult const& res) {
    return HandleFDv2PollResponse(res, protocol_handler_, logger_, kIdentity);
}

}  // namespace launchdarkly::server_side::data_systems
