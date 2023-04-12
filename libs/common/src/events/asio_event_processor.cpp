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
      summary_state_(std::chrono::system_clock::now()),
      flush_interval_(config.flush_interval()),
      timer_(io_),
      host_(endpoints.events_base_url()),  // TODO: parse and use host
      path_(config.path()),
      authorization_(std::move(authorization)),
      uuids_(),
      conns_(),
      inbox_capacity_(config.capacity()),
      inbox_size_(0),
      inbox_mutex_(),
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
    boost::asio::post(io_, [this, e = std::move(input_event)]() mutable {
        InboxDecrement();
        HandleSend(std::move(e));
    });
}

void AsioEventProcessor::HandleSend(InputEvent e) {
    std::vector<OutputEvent> output_events = Process(std::move(e));

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
    summary_state_ = Summarizer(std::chrono::system_clock::now());
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

    boost::json::value summary_event;
    if (!summary_state_.empty()) {
        summary_event = boost::json::value_from(
            Summary{summary_state_, std::chrono::system_clock::now()});
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

    auto events = boost::json::value_from(outbox_.Consume());
    if (!summary_event.is_null()) {
        events.as_array().push_back(summary_event);
    }

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

std::vector<OutputEvent> AsioEventProcessor::Process(InputEvent event) {
    std::vector<OutputEvent> out;
    std::visit(
        overloaded{
            [&](client::FeatureEventParams&& e) {
                summary_state_.update(e);

                if (!e.eval_result.track_events()) {
                    return;
                }
                std::optional<Reason> reason;

                // TODO(cwaldren): should also add the reason if the variation
                // method was VariationDetail().
                if (e.eval_result.track_reason()) {
                    reason = e.eval_result.detail().reason();
                }

                client::FeatureEventBase b = {
                    e.creation_date, std::move(e.key), e.eval_result.version(),
                    e.eval_result.detail().variation_index(),
                    e.eval_result.detail().value(), reason,
                    // TODO(cwaldren): change to actual default; figure out
                    // where this should be plumbed through.
                    Value::null()};

                auto debug_until_date = e.eval_result.debug_events_until_date();
                bool emit_debug_event =
                    debug_until_date &&
                    debug_until_date.value() > std::chrono::system_clock::now();

                if (emit_debug_event) {
                    out.emplace_back(
                        client::DebugEvent{b, filter_.filter(e.context)});
                }
                // TODO(cwaldren): see about not copying the keys / having the
                // getter return a value.
                out.emplace_back(client::FeatureEvent{
                    std::move(b), e.context.kinds_to_keys()});
            },
            [&](client::IdentifyEventParams&& e) {
                // Contexts should already have been checked for
                // validity by this point.
                assert(e.context.valid());
                out.emplace_back(client::IdentifyEvent{
                    e.creation_date, filter_.filter(e.context)});
            },
            [&](TrackEventParams&& e) { out.emplace_back(std::move(e)); }},
        std::move(event));

    return out;
}
}  // namespace launchdarkly::events::detail
