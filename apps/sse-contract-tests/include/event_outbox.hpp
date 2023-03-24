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

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class EventOutbox : public std::enable_shared_from_this<EventOutbox> {

    using RequestType = http::request<http::string_body>;

    std::string callback_url_;
    std::string callback_port_;
    std::string callback_host_;
    size_t callback_counter_;

    net::any_io_executor executor_;
    tcp::resolver resolver_;
    beast::tcp_stream event_stream_;

    boost::lockfree::spsc_queue<RequestType> outbox_;

    net::deadline_timer flush_timer_;
    std::string id_;

    bool shutdown_;

   public:
    /**
     * Instantiate an outbox; events will be posted to the given URL.
     * @param executor Executor.
     * @param callback_url Target URL.
     */
    EventOutbox(net::any_io_executor executor, std::string callback_url);
    /**
     * Enqueues an event, which will be posted to the server
     * later.
     * @param event Event to post.
     */
    void post_event(launchdarkly::sse::Event event);
    /**
     * Begins an async operation to connect to the server.
     */
    void run();
    /**
     * Begins an async operation to disconnect from the server.
     */
    void stop();

   private:
    RequestType build_request(std::size_t counter,
                               launchdarkly::sse::Event ev);
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec,
                    tcp::resolver::results_type::endpoint_type);
    void on_flush_timer(boost::system::error_code ec);
    void on_write(beast::error_code ec, std::size_t);
    void do_shutdown(beast::error_code ec, std::string what);
};
