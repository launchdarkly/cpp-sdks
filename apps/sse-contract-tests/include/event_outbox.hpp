#pragma once

#include "entity_manager.hpp"

#include <launchdarkly/sse/client.hpp>

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#include <memory>
#include <string>

namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

class EventOutbox : public std::enable_shared_from_this<EventOutbox> {
    // Simple string body request is appropriate since the JSON
    // returned to the test service is minimal.
    using request_type = http::request<http::string_body>;

    std::string callback_url_;
    std::string callback_port_;
    std::string callback_host_;
    size_t callback_counter_;

    net::any_io_executor executor_;
    tcp::resolver resolver_;
    beast::tcp_stream event_stream_;

    // When events are received from the SSE client, they are pushed into
    // this queue.
    boost::lockfree::spsc_queue<request_type> outbox_;
    // Periodically, the events are flushed to the test harness.
    net::deadline_timer flush_timer_;
    std::string id_;

   public:
    EventOutbox(net::any_io_executor executor, std::string callback_url);

    void deliver_event(launchdarkly::sse::Event event);

    void run();
    void stop();

   private:
    request_type build_request(std::size_t counter,
                               launchdarkly::sse::Event ev);
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec,
                    tcp::resolver::results_type::endpoint_type);
    void on_flush_timer(boost::system::error_code ec);
    void on_write(beast::error_code ec, std::size_t);

    void do_shutdown(beast::error_code ec, std::string what);
};
