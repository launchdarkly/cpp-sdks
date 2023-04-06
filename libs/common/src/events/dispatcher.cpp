#include "events/detail/dispatcher.hpp"
#include <boost/asio/post.hpp>
#include <boost/beast/http.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace http = boost::beast::http;
namespace launchdarkly::events::detail {

auto const kEventSchemaHeader = "X-LaunchDarkly-Event-Schema";
auto const kPayloadIdHeader = "X-LaunchDarkly-Payload-Id";

auto const kEventSchemaVersion = 4;

Dispatcher::Dispatcher(boost::asio::any_io_executor io,
                       std::size_t outbox_capacity,
                       std::chrono::milliseconds min_flush_interval,
                       std::string endpoint_host,
                       std::string endpoint_path,
                       std::string authorization,
                       Logger& logger)
    : io_(std::move(io)),
      work_guard_(io_),
      outbox_(outbox_capacity),
      summary_state_(std::chrono::system_clock::now()),
      min_flush_interval_(min_flush_interval),
      last_flush_(std::chrono::system_clock::now()),
      host_(endpoint_host),
      path_(endpoint_path),
      authorization_(authorization),
      uuids_(),
      full_outbox_encountered_(false),
      logger_(logger) {}

void Dispatcher::send(InputEvent input_event) {
    boost::asio::post(io_, [this, input_event]() { handle_send(input_event); });
}

void Dispatcher::handle_send(InputEvent input_event) {
    summary_state_.update(input_event);

    std::vector<OutputEvent> output_events = process(std::move(input_event));

    bool full = outbox_.push_discard_overflow(std::move(output_events));
    if (full && !full_outbox_encountered_) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "event-processor: exceeded event queue capacity; increase "
               "capacity to avoid dropping events";
    }
    full_outbox_encountered_ = full;

    if (flush_due()) {
        flush(std::chrono::system_clock::now());
    }
}

void Dispatcher::flush(std::chrono::system_clock::time_point when) {
    if (auto request = make_request()) {
        conns_.async_write(*request);
    }
    last_flush_ = when;
}

void Dispatcher::shutdown() {
    work_guard_.reset();
}

void Dispatcher::request_flush() {
    boost::asio::post(io_, [this] { flush(last_flush_); });
}

std::optional<Dispatcher::RequestType> Dispatcher::make_request() {
    LD_LOG(logger_, LogLevel::kDebug) << "generating http request";
    RequestType req;

    req.set(http::field::host, host_);
    req.method(http::verb::post);
    req.set(http::field::content_type, "application/json");
    req.set(http::field::authorization, authorization_);
    req.set(kEventSchemaHeader, std::to_string(kEventSchemaVersion));
    req.set(kPayloadIdHeader, boost::lexical_cast<std::string>(uuids_()));
    req.target(host_ + path_);

    req.body() = "[]";
    req.prepare_payload();
    return req;
}

bool Dispatcher::flush_due() {
    return std::chrono::system_clock::now() - last_flush_ >=
           min_flush_interval_;
}

std::vector<OutputEvent> Dispatcher::process(InputEvent e) {
    LD_LOG(logger_, LogLevel::kDebug) << "dispatcher: processing an event";
    return {};
}

}  // namespace launchdarkly::events::detail
