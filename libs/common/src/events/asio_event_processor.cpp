#include "events/detail/asio_event_processor.hpp"
#include <boost/asio/post.hpp>
#include <boost/beast/http.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "serialization/events/json_events.hpp"

namespace http = boost::beast::http;
namespace launchdarkly::events::detail {

auto const kEventSchemaHeader = "X-LaunchDarkly-Event-Schema";
auto const kPayloadIdHeader = "X-LaunchDarkly-Payload-Id";

auto const kEventSchemaVersion = 4;

AsioEventProcessor::AsioEventProcessor(boost::asio::any_io_executor io,
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
      filter_(config.all_attributes_private(), config.private_attributes()),
      logger_(logger) {
    ScheduleFlush();
}

void AsioEventProcessor::AsyncSend(InputEvent input_event) {
    boost::asio::post(io_, [this, e = std::move(input_event)]() mutable {
        HandleSend(std::move(e));
    });
}

void AsioEventProcessor::HandleSend(InputEvent e) {
    summary_state_.update(e);

    std::vector<OutputEvent> output_events = Process(std::move(e));

    bool inserted = outbox_.push_discard_overflow(std::move(output_events));
    if (!inserted && !full_outbox_encountered_) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "event-processor: exceeded event queue capacity; increase "
               "capacity to avoid dropping events";
    }
    full_outbox_encountered_ = !inserted;
}

void AsioEventProcessor::Flush(FlushTrigger flush_type) {
    if (auto request = MakeRequest()) {
        conns_.async_write(*request);
    } else {
        LD_LOG(logger_, LogLevel::kDebug)
            << "event-processor: nothing to flush";
    }
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

void AsioEventProcessor::AsyncClose() {
    timer_.cancel();
}

void AsioEventProcessor::AsyncFlush() {
    boost::asio::post(io_, [this] {
        boost::system::error_code ec;
        Flush(FlushTrigger::Manual);
    });
}

std::optional<AsioEventProcessor::RequestType>
AsioEventProcessor::MakeRequest() {
    if (outbox_.empty()) {
        return std::nullopt;
    }

    LD_LOG(logger_, LogLevel::kDebug) << "generating http request";
    RequestType req;

    req.set(http::field::host, host_);
    req.method(http::verb::post);
    req.set(http::field::content_type, "application/json");
    req.set(http::field::authorization, authorization_);
    req.set(kEventSchemaHeader, std::to_string(kEventSchemaVersion));
    req.set(kPayloadIdHeader, boost::lexical_cast<std::string>(uuids_()));
    req.target(host_ + path_);

    req.body() =
        boost::json::serialize(boost::json::value_from(outbox_.consume()));
    req.prepare_payload();
    return req;
}

static std::map<std::string, std::string> CopyContextKeys(
    std::map<std::string_view, std::string_view> const& refs) {
    std::map<std::string, std::string> copied_keys;
    for (auto kv : refs) {
        copied_keys.insert(kv);
    }
    return copied_keys;
}

// These helpers are for the std::visit within Dispatcher::process.
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
                    std::move(b), CopyContextKeys(e.context.kinds_to_keys())});
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
