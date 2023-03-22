#pragma once

#include <launchdarkly/sse/sse.hpp>

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#include <memory>
#include <string>

namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

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
                  std::string callback_url);

    void run();
    void stop();
   private:
    request_type build_request(std::size_t counter,
                               launchdarkly::sse::event_data ev);
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec,
                    tcp::resolver::results_type::endpoint_type);
    void on_flush_timer(boost::system::error_code ec);
    void on_write(beast::error_code ec, std::size_t);
};
