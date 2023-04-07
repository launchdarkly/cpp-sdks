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
                       config::detail::Events const& config,
                       config::ServiceHosts const& endpoints,
                       std::string authorization,
                       Logger& logger)
    : io_(std::move(io)),
      outbox_(config.capacity()),
      summary_state_(std::chrono::system_clock::now()),
      flush_interval_(config.flush_interval()),
      timer_(io_),
      host_(endpoints.events_host()),
      path_(config.path()),
      authorization_(std::move(authorization)),
      uuids_(),
      full_outbox_encountered_(false),
      logger_(logger) {
    schedule_flush();
}

void Dispatcher::send(InputEvent input_event) {
    boost::asio::post(io_, [this, e = std::move(input_event)]() mutable {
        handle_send(std::move(e));
    });
}

void Dispatcher::handle_send(InputEvent input_event) {
    summary_state_.update(input_event);

    std::vector<OutputEvent> output_events = process(std::move(input_event));

    bool inserted = outbox_.push_discard_overflow(std::move(output_events));
    if (!inserted && !full_outbox_encountered_) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "event-processor: exceeded event queue capacity; increase "
               "capacity to avoid dropping events";
    }
    full_outbox_encountered_ = !inserted;
}

void Dispatcher::flush(FlushTrigger flush_type) {
    if (auto request = make_request()) {
        conns_.async_write(*request);
    } else {
        LD_LOG(logger_, LogLevel::kDebug) << "dispatcher: nothing to flush";
    }
    if (flush_type == FlushTrigger::Automatic) {
        schedule_flush();
    }
}

void Dispatcher::schedule_flush() {
    LD_LOG(logger_, LogLevel::kDebug) << "dispatcher: scheduling flush in "
                                      << flush_interval_.count() << "ms";

    timer_.expires_from_now(flush_interval_);
    timer_.async_wait([this](boost::system::error_code ec) {
        if (ec) {
            LD_LOG(logger_, LogLevel::kDebug) << "dispatcher: flush cancelled";
            return;
        }
        LD_LOG(logger_, LogLevel::kDebug) << "dispatcher: flush";
        flush(FlushTrigger::Automatic);
    });
}

void Dispatcher::shutdown() {
    timer_.cancel();
}

void Dispatcher::request_flush() {
    boost::asio::post(io_, [this] {
        boost::system::error_code ec;
        flush(FlushTrigger::Manual);
    });
}

static boost::json serialize(std::vector<OutputEvent> events) {

}

std::optional<Dispatcher::RequestType> Dispatcher::make_request() {

    if (outbox_.empty()) {
        return std::nullopt;
    }

    auto json = serialize(outbox_.consume());

    LD_LOG(logger_, LogLevel::kDebug) << "generating http request";
    RequestType req;

    req.set(http::field::host, host_);
    req.method(http::verb::post);
    req.set(http::field::content_type, "application/json");
    req.set(http::field::authorization, authorization_);
    req.set(kEventSchemaHeader, std::to_string(kEventSchemaVersion));
    req.set(kPayloadIdHeader, boost::lexical_cast<std::string>(uuids_()));
    req.target(host_ + path_);

    req.body() = json;
    req.prepare_payload();
    return req;
}

std::vector<OutputEvent> Dispatcher::process(InputEvent e) {
    LD_LOG(logger_, LogLevel::kDebug) << "dispatcher: processing an event";
    return {};
}

}  // namespace launchdarkly::events::detail
