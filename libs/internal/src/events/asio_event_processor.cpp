#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/http.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <launchdarkly/config/shared/builders/http_properties_builder.hpp>
#include <launchdarkly/config/shared/sdks.hpp>
#include <launchdarkly/events/asio_event_processor.hpp>
#include <launchdarkly/network/asio_requester.hpp>
#include <launchdarkly/serialization/events/json_events.hpp>

namespace http = boost::beast::http;
namespace launchdarkly::events {

auto const kEventSchemaHeader = "X-LaunchDarkly-Event-Schema";
auto const kPayloadIdHeader = "X-LaunchDarkly-Payload-Id";
auto const kEventSchemaVersion = 4;

// These helpers are for usage with std::visit.
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template <typename SDK>
AsioEventProcessor<SDK>::AsioEventProcessor(
    boost::asio::any_io_executor const& io,
    config::Config<SDK> const& config,
    Logger& logger)
    : io_(boost::asio::make_strand(io)),
      outbox_(config.Events().Capacity()),
      summarizer_(std::chrono::system_clock::now()),
      flush_interval_(config.Events().FlushInterval()),
      timer_(io_),
      url_(config.ServiceEndpoints().EventsBaseUrl() + config.Events().Path()),
      http_props_(config.HttpProperties()),
      authorization_(config.SdkKey()),
      uuids_(),
      workers_(io_,
               config.Events().FlushWorkers(),
               config.Events().DeliveryRetryDelay(),
               logger),
      inbox_capacity_(config.Events().Capacity()),
      inbox_size_(0),
      full_outbox_encountered_(false),
      full_inbox_encountered_(false),
      permanent_delivery_failure_(false),
      last_known_past_time_(std::nullopt),
      filter_(config.Events().AllAttributesPrivate(),
              config.Events().PrivateAttributes()),
      logger_(logger) {
    ScheduleFlush();
}

template <typename SDK>
bool AsioEventProcessor<SDK>::InboxIncrement() {
    std::lock_guard<std::mutex> guard{inbox_mutex_};
    if (permanent_delivery_failure_) {
        return false;
    }
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

template <typename SDK>
void AsioEventProcessor<SDK>::InboxDecrement() {
    std::lock_guard<std::mutex> guard{inbox_mutex_};
    if (inbox_size_ > 0) {
        inbox_size_--;
    }
}

template <typename SDK>
void AsioEventProcessor<SDK>::AsyncSend(InputEvent input_event) {
    if (!InboxIncrement()) {
        return;
    }
    boost::asio::post(io_, [this, event = std::move(input_event)]() mutable {
        InboxDecrement();
        HandleSend(std::move(event));
    });
}

template <typename SDK>
void AsioEventProcessor<SDK>::HandleSend(InputEvent event) {
    std::vector<OutputEvent> output_events = Process(std::move(event));

    bool inserted = outbox_.PushDiscardingOverflow(std::move(output_events));
    if (!inserted && !full_outbox_encountered_) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "event-processor: exceeded event queue capacity; increase "
               "capacity to avoid dropping events";
    }
    full_outbox_encountered_ = !inserted;
}

template <typename SDK>
void AsioEventProcessor<SDK>::Flush(FlushTrigger flush_type) {
    workers_.Get([this](RequestWorker* worker) {
        if (worker == nullptr) {
            LD_LOG(logger_, LogLevel::kDebug)
                << "event-processor: no flush workers available; skipping "
                   "flush";
            return;
        }
        auto batch = CreateBatch();
        if (!batch) {
            LD_LOG(logger_, LogLevel::kDebug)
                << "event-processor: nothing to flush";
            return;
        }
        worker->AsyncDeliver(
            std::move(*batch),
            [this](std::size_t count, RequestWorker::DeliveryResult result) {
                OnEventDeliveryResult(count, result);
            });
        summarizer_ = Summarizer(Clock::now());
    });

    if (flush_type == FlushTrigger::Automatic) {
        ScheduleFlush();
    }
}

template <typename SDK>
void AsioEventProcessor<SDK>::OnEventDeliveryResult(
    std::size_t event_count,
    RequestWorker::DeliveryResult result) {
    boost::ignore_unused(event_count);

    std::visit(
        overloaded{[&](Clock::time_point server_time) {
                       last_known_past_time_ = server_time;
                   },
                   [&](network::HttpResult::StatusCode status) {
                       std::lock_guard<std::mutex> guard{this->inbox_mutex_};
                       if (!permanent_delivery_failure_) {
                           timer_.cancel();
                           permanent_delivery_failure_ = true;
                       }
                   }},
        result);
}

template <typename SDK>
void AsioEventProcessor<SDK>::ScheduleFlush() {
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

template <typename SDK>
void AsioEventProcessor<SDK>::AsyncFlush() {
    boost::asio::post(io_, [this] {
        boost::system::error_code ec;
        Flush(FlushTrigger::Manual);
    });
}

template <typename SDK>
void AsioEventProcessor<SDK>::AsyncClose() {
    timer_.cancel();
}

template <typename SDK>
std::optional<EventBatch> AsioEventProcessor<SDK>::CreateBatch() {
    auto events = boost::json::value_from(outbox_.Consume()).as_array();

    bool has_summary =
        !summarizer_.Finish(std::chrono::system_clock::now()).Empty();

    if (has_summary) {
        events.push_back(boost::json::value_from(summarizer_));
    } else if (events.empty()) {
        return std::nullopt;
    }

    // TODO(cwaldren): Template the event processor over SDK type? Add it into
    // HttpProperties?
    config::shared::builders::HttpPropertiesBuilder<config::shared::ClientSDK>
        props(http_props_);

    props.Header(kEventSchemaHeader, std::to_string(kEventSchemaVersion));
    props.Header(kPayloadIdHeader, boost::lexical_cast<std::string>(uuids_()));
    props.Header(to_string(http::field::authorization), authorization_);
    props.Header(to_string(http::field::content_type), "application/json");

    return EventBatch(url_, props.Build(), events);
}

template <typename SDK>
std::vector<OutputEvent> AsioEventProcessor<SDK>::Process(
    InputEvent input_event) {
    std::vector<OutputEvent> out;
    std::visit(
        overloaded{[&](client::FeatureEventParams&& event) {
                       summarizer_.Update(event);

                       client::FeatureEventBase base{event};

                       auto debug_until_date = event.debug_events_until_date;

                       // To be conservative, use as the current time the
                       // maximum of the actual current time and the server's
                       // time. This way if the local host is running behind, we
                       // won't accidentally keep emitting events.

                       auto conservative_now = std::max(
                           std::chrono::system_clock::now(),
                           last_known_past_time_.value_or(
                               std::chrono::system_clock::from_time_t(0)));

                       bool emit_debug_event =
                           debug_until_date &&
                           conservative_now < debug_until_date->t;

                       if (emit_debug_event) {
                           out.emplace_back(client::DebugEvent{
                               base, filter_.filter(event.context)});
                       }

                       if (event.require_full_event) {
                           out.emplace_back(client::FeatureEvent{
                               std::move(base), event.context.kinds_to_keys()});
                       }
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

template class AsioEventProcessor<config::shared::ClientSDK>;

}  // namespace launchdarkly::events
