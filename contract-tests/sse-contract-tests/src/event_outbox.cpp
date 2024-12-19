#include "event_outbox.hpp"
#include "definitions.hpp"

#include <boost/beast/core/bind_handler.hpp>
#include <boost/url/parse.hpp>
#include <nlohmann/json.hpp>

#include <iostream>

// Check the outbox at this interval. Normally a flush is triggered for
// every event; this is just a failsafe in case a flush is happening
// concurrently.
auto const kFlushInterval = boost::posix_time::milliseconds{500};
auto const kOutboxCapacity = 1023;

EventOutbox::EventOutbox(net::any_io_executor executor,
                         std::string callback_url)
    : callback_url_{std::move(callback_url)},
      callback_counter_{0},
      executor_{executor},
      resolver_{executor},
      event_stream_{executor},
      outbox_{kOutboxCapacity},
      flush_timer_{executor},
      shutdown_(false) {
    auto uri_components = boost::urls::parse_uri(callback_url_);

    callback_host_ = uri_components->host();
    callback_port_ = uri_components->port();
}

void EventOutbox::do_shutdown(beast::error_code ec) {
    event_stream_.socket().shutdown(tcp::socket::shutdown_both, ec);
    flush_timer_.cancel();
}

void EventOutbox::post_event(launchdarkly::sse::Event event) {
    auto http_request = build_request(callback_counter_++, std::move(event));
    outbox_.push(http_request);
    flush_timer_.expires_after(boost::posix_time::milliseconds(0));
}

void EventOutbox::post_error(launchdarkly::sse::Error error) {
    auto http_request = build_request(callback_counter_++, error);
    outbox_.push(http_request);
    flush_timer_.expires_after(boost::posix_time::milliseconds(0));
}

void EventOutbox::run() {
    resolver_.async_resolve(callback_host_, callback_port_,
                            beast::bind_front_handler(&EventOutbox::on_resolve,
                                                      shared_from_this()));
}

void EventOutbox::stop() {
    beast::error_code ec = net::error::basic_errors::operation_aborted;
    shutdown_ = true;
    net::post(executor_, beast::bind_front_handler(&EventOutbox::do_shutdown,
                                                   shared_from_this(), ec));
}

EventOutbox::RequestType EventOutbox::build_request(
    std::size_t counter,
    std::variant<launchdarkly::sse::Event, launchdarkly::sse::Error> event) {
    RequestType req;

    req.set(http::field::host, callback_host_);
    req.method(http::verb::post);
    req.target(callback_url_ + "/" + std::to_string(counter));

    nlohmann::json json;

    std::visit(
        [&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, launchdarkly::sse::Event>) {
                if (arg.type() == "comment") {
                    json = CommentMessage{"comment", std::move(arg).take()};
                } else {
                    json = EventMessage{"event", Event{std::move(arg)}};
                }
            } else if constexpr (std::is_same_v<T, launchdarkly::sse::Error>) {
                auto msg = ErrorMessage{"error",
                                        launchdarkly::sse::ErrorToString(arg)};
                json = msg;
            }
        },
        std::move(event));

    req.body() = json.dump();
    req.prepare_payload();
    return req;
}

void EventOutbox::on_resolve(beast::error_code ec,
                             tcp::resolver::results_type results) {
    if (ec) {
        return do_shutdown(ec);
    }

    beast::get_lowest_layer(event_stream_)
        .async_connect(results,
                       beast::bind_front_handler(&EventOutbox::on_connect,
                                                 shared_from_this()));
}

void EventOutbox::on_connect(beast::error_code ec,
                             tcp::resolver::results_type::endpoint_type) {
    if (ec) {
        return do_shutdown(ec);
    }

    boost::system::error_code dummy;
    net::post(executor_, beast::bind_front_handler(&EventOutbox::on_flush_timer,
                                                   shared_from_this(), dummy));
}

void EventOutbox::on_flush_timer(boost::system::error_code ec) {
    if (ec && shutdown_) {
        return do_shutdown(ec);
    }

    if (!outbox_.empty()) {
        RequestType& request = outbox_.front();

        // Flip-flop between this function and on_write; peeking an event
        // and then popping it.

        http::async_write(event_stream_, request,
                          beast::bind_front_handler(&EventOutbox::on_write,
                                                    shared_from_this()));
        return;
    }

    // If the outbox is empty, wait a bit before trying again.
    flush_timer_.expires_after(kFlushInterval);
    flush_timer_.async_wait(beast::bind_front_handler(
        &EventOutbox::on_flush_timer, shared_from_this()));
}

void EventOutbox::on_write(beast::error_code ec, std::size_t) {
    if (ec) {
        return do_shutdown(ec);
    }
    outbox_.pop();
    on_flush_timer(boost::system::error_code{});
}
