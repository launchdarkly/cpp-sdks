#pragma once

#include "definitions.hpp"

#include <boost/asio/placeholders.hpp>
#include <boost/bind/bind.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/url/parse.hpp>
#include <deque>
#include <iostream>
#include <launchdarkly/sse/sse.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

void fail(beast::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

const auto kFlushInterval = boost::posix_time::milliseconds {100};

class stream_entity : public std::enable_shared_from_this<stream_entity> {
    // Simple string body request is appropriate since the JSON
    // returned to the test service is minimal.
    using request_type = http::request<http::string_body>;

    std::shared_ptr<launchdarkly::sse::client> client_;

    std::string callback_url_;
    std::string callback_port_;
    std::string callback_host_;
    size_t callback_counter_;

    net::any_io_executor executor_;
    tcp::resolver resolver_;
    beast::tcp_stream stream_;

    // When events are received from the SSE client, they are pushed into
    // this queue.
    boost::lockfree::spsc_queue<request_type> outbox_;
    // Periodically, the events are flushed to the test harness.
    net::deadline_timer flush_timer_;

   public:
    stream_entity(net::any_io_executor executor,
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

    void run() {
        // Setup the SSE client to callback into the entity whenever it
        // receives a comment/event.
        client_->on_event(
            [self = shared_from_this()](launchdarkly::sse::event_data ev) {
                auto http_request = self->build_request(
                    self->callback_counter_++, std::move(ev));
                self->outbox_.push(http_request);
            });

        // Kickoff the SSE client's async operations.
        client_->run();

        // Immediately kickoff the flush "loop".
        flush_timer_.async_wait(beast::bind_front_handler(
            &stream_entity::on_flush_timer, shared_from_this()));
    }

    void stop() {
        flush_timer_.cancel();
        client_->close();
    }

   private:
    request_type build_request(std::size_t counter,
                               launchdarkly::sse::event_data ev) {
        request_type req;

        req.set(http::field::host, callback_host_);
        req.method(http::verb::get);
        req.target(callback_url_ + "/" + std::to_string(counter));

        nlohmann::json json;

        if (ev.get_type() == "comment") {
            json = comment_message{"comment", ev.get_data()};
        } else {
            json = event_message{"event", event{ev.get_type(), ev.get_data(),
                                                ev.get_id().value_or("")}};
        }

        req.body() = json.dump();
        req.prepare_payload();
        return req;
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec)
            return fail(ec, "resolve");
        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(stream_).async_connect(
            results, beast::bind_front_handler(&stream_entity::on_connect,
                                               shared_from_this()));
    }

    void on_connect(beast::error_code ec,
                    tcp::resolver::results_type::endpoint_type) {
        if (ec)
            return fail(ec, "connect");

        request_type& req = outbox_.front();
        http::async_write(stream_, req,
                          beast::bind_front_handler(&stream_entity::on_write,
                                                    shared_from_this()));
    }

    void on_write(beast::error_code ec, std::size_t) {
        if (ec)
            return fail(ec, "write");

        outbox_.pop();
        stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes so don't bother reporting it.
        if (ec && ec != beast::errc::not_connected)
            return fail(ec, "shutdown");

        // If there's more to do, queue up another resolve. Otherwise,
        // wait a little while before trying again.
        if (!outbox_.empty()) {
            resolver_.async_resolve(
                callback_host_, callback_port_,
                beast::bind_front_handler(&stream_entity::on_resolve,
                                          shared_from_this()));
        } else {
            flush_timer_.expires_from_now(kFlushInterval);
            flush_timer_.async_wait(beast::bind_front_handler(
                &stream_entity::on_flush_timer, shared_from_this()));
        }
    }

    void on_flush_timer(boost::system::error_code const& ec) {
        if (ec) {
            return fail(ec, "flush");
        }

        if (!outbox_.empty()) {
            resolver_.async_resolve(
                callback_host_, callback_port_,
                beast::bind_front_handler(&stream_entity::on_resolve,
                                          shared_from_this()));
        } else {
            flush_timer_.expires_from_now(kFlushInterval);
            flush_timer_.async_wait(beast::bind_front_handler(
                &stream_entity::on_flush_timer, shared_from_this()));
        }
    }
};
