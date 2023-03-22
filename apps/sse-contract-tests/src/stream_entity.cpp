#include "stream_entity.hpp"
#include "definitions.hpp"

#include <boost/url/parse.hpp>
#include <iostream>
#include <nlohmann/json.hpp>

void fail(beast::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

// Periodically check the outbox (outgoing events to the test harness)
// at this interval.
auto const kFlushInterval = boost::posix_time::milliseconds{10};

StreamEntity::StreamEntity(net::any_io_executor executor,
                             std::shared_ptr<launchdarkly::sse::client> client,
                             std::string callback_url)
    : client_{std::move(client)},
      callback_url_{std::move(callback_url)},
      callback_port_{},
      callback_host_{},
      callback_counter_{0},
      executor_{executor},
      resolver_{executor},
      stream_{executor},
      outbox_{1024},
      flush_timer_{executor, boost::posix_time::milliseconds{0}} {
    boost::system::result<boost::urls::url_view> uri_components =
        boost::urls::parse_uri(callback_url_);

    callback_host_ = uri_components->host();
    callback_port_ = uri_components->port();
}

void StreamEntity::run() {
    // Setup the SSE client to callback into the entity whenever it
    // receives a comment/event.
    client_->on_event(
        [self = shared_from_this()](launchdarkly::sse::event_data ev) {
            auto http_request =
                self->build_request(self->callback_counter_++, std::move(ev));
            self->outbox_.push(http_request);
        });

    // Kickoff the SSE client's async operations.
    client_->run();

    // Begin connecting to the test harness's event-posting service.
    resolver_.async_resolve(
        callback_host_, callback_port_,
        beast::bind_front_handler(&StreamEntity::on_resolve,
                                  shared_from_this()));
}

void StreamEntity::stop() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

    flush_timer_.cancel();
    client_->close();
}

StreamEntity::request_type StreamEntity::build_request(
    std::size_t counter,
    launchdarkly::sse::event_data ev) {
    request_type req;

    req.set(http::field::host, callback_host_);
    req.method(http::verb::get);
    req.target(callback_url_ + "/" + std::to_string(counter));

    nlohmann::json json;

    if (ev.get_type() == "comment") {
        json = CommentMessage{"comment", ev.get_data()};
    } else {
        json = EventMessage{"event", Event{ev.get_type(), ev.get_data(),
                                            ev.get_id().value_or("")}};
    }

    req.body() = json.dump();
    req.prepare_payload();
    return req;
}

void StreamEntity::on_resolve(beast::error_code ec,
                               tcp::resolver::results_type results) {
    if (ec)
        return fail(ec, "resolve");

    // Make the connection on the IP address we get from a lookup.
    beast::get_lowest_layer(stream_).async_connect(
        results, beast::bind_front_handler(&StreamEntity::on_connect,
                                           shared_from_this()));
}

void StreamEntity::on_connect(beast::error_code ec,
                               tcp::resolver::results_type::endpoint_type) {
    if (ec)
        return fail(ec, "connect");

    // Now that we're connected, kickoff the event flush "loop".
    boost::system::error_code dummy;
    net::post(executor_,
              beast::bind_front_handler(&StreamEntity::on_flush_timer,
                                        shared_from_this(), dummy));
}

void StreamEntity::on_flush_timer(boost::system::error_code ec) {
    if (ec && ec != net::error::operation_aborted) {
        return fail(ec, "flush");
    }

    if (!outbox_.empty()) {
        request_type& request = outbox_.front();

        // Flip-flop between this function and on_write; pushing an event
        // and then popping it.
        http::async_write(stream_, request,
                          beast::bind_front_handler(&StreamEntity::on_write,
                                                    shared_from_this()));
        return;
    }

    // If the outbox is empty, wait a bit before trying again.
    flush_timer_.expires_from_now(kFlushInterval);
    flush_timer_.async_wait(beast::bind_front_handler(
        &StreamEntity::on_flush_timer, shared_from_this()));
}

void StreamEntity::on_write(beast::error_code ec, std::size_t) {
    if (ec)
        return fail(ec, "write");

    outbox_.pop();
    on_flush_timer(boost::system::error_code{});
}
