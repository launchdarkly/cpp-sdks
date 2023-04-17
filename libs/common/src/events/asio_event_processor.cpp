#include "events/detail/asio_event_processor.hpp"
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/http.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "serialization/events/json_events.hpp"

namespace http = boost::beast::http;
namespace launchdarkly::events::detail {

auto const kEventSchemaHeader = "X-LaunchDarkly-Event-Schema";
auto const kPayloadIdHeader = "X-LaunchDarkly-Payload-Id";
auto const kEventSchemaVersion = 4;

AsioEventProcessor::AsioEventProcessor(
    boost::asio::any_io_executor const& io,
    config::detail::Events const& config,
    config::ServiceEndpoints const& endpoints,
    std::string authorization,
    Logger& logger)
    : io_(boost::asio::make_strand(io)),
      outbox_(config.capacity()),
      summarizer_(std::chrono::system_clock::now()),
      flush_interval_(config.flush_interval()),
      timer_(io_),
      host_(endpoints.EventsBaseUrl()),  // TODO: parse and use host
      path_(config.path()),
      authorization_(std::move(authorization)),
      uuids_(),
      inbox_capacity_(config.capacity()),
      inbox_size_(0),
      full_outbox_encountered_(false),
      full_inbox_encountered_(false),
      filter_(config.all_attributes_private(), config.private_attributes()),
      logger_(logger) {
    ScheduleFlush();
}

bool AsioEventProcessor::InboxIncrement() {
    std::lock_guard<std::mutex> guard{inbox_mutex_};
    if (inbox_size_ < inbox_capacity_) {
        inbox_size_++;
        return true;
    }
    if (!full_inbox_encountered_) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "event-processor: events are being produced faster than they "
               "can be "
               "processed; some events will be dropped";
        full_inbox_encountered_ = true;
    }
    return false;
}

void AsioEventProcessor::InboxDecrement() {
    std::lock_guard<std::mutex> guard{inbox_mutex_};
    if (inbox_size_ > 0) {
        inbox_size_--;
    }
}

void AsioEventProcessor::AsyncSend(InputEvent input_event) {
    if (!InboxIncrement()) {
        return;
    }
    boost::asio::post(io_, [this, event = std::move(input_event)]() mutable {
        InboxDecrement();
        HandleSend(std::move(event));
    });
}

void AsioEventProcessor::HandleSend(InputEvent event) {
    std::vector<OutputEvent> output_events = Process(std::move(event));

    bool inserted = outbox_.PushDiscardingOverflow(std::move(output_events));
    if (!inserted && !full_outbox_encountered_) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "event-processor: exceeded event queue capacity; increase "
               "capacity to avoid dropping events";
    }
    full_outbox_encountered_ = !inserted;
}

void AsioEventProcessor::Flush(FlushTrigger flush_type) {
    if (auto request = MakeRequest()) {
        conns_.Deliver(*request);
    } else {
        LD_LOG(logger_, LogLevel::kDebug)
            << "event-processor: nothing to flush";
    }
    summarizer_ = Summarizer(std::chrono::system_clock::now());
    if (flush_type == FlushTrigger::Automatic) {
        ScheduleFlush();
    }
}

void AsioEventProcessor::ScheduleFlush() {
    LD_LOG(logger_, LogLevel::kDebug) << "event-processor: scheduling flush in "
                                      << flush_interval_.count() << "ms";

    timer_.expires_from_now(flush_interval_);
    timer_.async_wait([this](boost::system::error_code ec) {
        if (ec) {
            LD_LOG(logger_, LogLevel::kDebug)
                << "event-processor: flush cancelled";
            return;
        }
        LD_LOG(logger_, LogLevel::kDebug) << "event-processor: flush";
        Flush(FlushTrigger::Automatic);
    });
}

void AsioEventProcessor::AsyncFlush() {
    boost::asio::post(io_, [this] {
        boost::system::error_code ec;
        Flush(FlushTrigger::Manual);
    });
}

void AsioEventProcessor::AsyncClose() {
    timer_.cancel();
}

std::optional<AsioEventProcessor::RequestType>
AsioEventProcessor::MakeRequest() {
    if (outbox_.Empty()) {
        return std::nullopt;
    }

    auto events = boost::json::value_from(outbox_.Consume());

    if (!summarizer_.Finish(std::chrono::system_clock::now()).Empty()) {
        events.as_array().push_back(boost::json::value_from(summarizer_));
    }

    LD_LOG(logger_, LogLevel::kDebug)
        << "event-processor: generating http request";

    RequestType req;

    req.set(http::field::host, host_);
    req.method(http::verb::post);
    req.set(http::field::content_type, "application/json");
    req.set(http::field::authorization, authorization_);
    req.set(kEventSchemaHeader, std::to_string(kEventSchemaVersion));
    req.set(kPayloadIdHeader, boost::lexical_cast<std::string>(uuids_()));
    req.target(host_ + path_);

    req.body() = boost::json::serialize(events);
    req.prepare_payload();
    return req;
}

// These helpers are for the std::visit within AsioEventProcessor::Process.
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

std::vector<OutputEvent> AsioEventProcessor::Process(InputEvent input_event) {
    std::vector<OutputEvent> out;
    std::visit(
        overloaded{[&](client::FeatureEventParams&& event) {
                       summarizer_.Update(event);

                       if (!event.eval_result.track_events()) {
                           return;
                       }

                       client::FeatureEventBase base{event};

                       auto debug_until_date =
                           event.eval_result.debug_events_until_date();

                       bool emit_debug_event =
                           debug_until_date &&
                           debug_until_date.value() >
                               std::chrono::system_clock::now();

                       if (emit_debug_event) {
                           out.emplace_back(client::DebugEvent{
                               base, filter_.filter(event.context)});
                       }
                       out.emplace_back(client::FeatureEvent{
                           std::move(base), event.context.kinds_to_keys()});
                   },
                   [&](client::IdentifyEventParams&& event) {
                       // Contexts should already have been checked for
                       // validity by this point.
                       assert(event.context.valid());
                       out.emplace_back(client::IdentifyEvent{
                           event.creation_date, filter_.filter(event.context)});
                   },
                   [&](TrackEventParams&& event) {
                       out.emplace_back(std::move(event));
                   }},
        std::move(input_event));

    return out;
}
}  // namespace launchdarkly::events::detail
