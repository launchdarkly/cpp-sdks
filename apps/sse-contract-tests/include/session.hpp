#pragma once

#include "entity_manager.hpp"
#include "logger.hpp"

#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <vector>

namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

class Session : public std::enable_shared_from_this<Session> {
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_{8192};
    http::request<http::string_body> request_;
    EntityManager& manager_;
    std::vector<std::string> capabilities_;
    std::function<void()> on_shutdown_cb_;
    bool shutdown_requested_;
    launchdarkly::Logger& logger_;

   public:
    /**
     * Constructs a session, which provides a REST API.
     * @param socket Connected socket.
     * @param manager Manager through which entities can be created/destroyed.
     * @param caps Test service capabilities to advertise.
     * @param logger Logger.
     */
    explicit Session(tcp::socket&& socket,
                     EntityManager& manager,
                     std::vector<std::string> caps,
                     launchdarkly::Logger& logger);

    ~Session();

    /**
     * Set a callback to be invoked when a REST client requests shutdown.
     */
    template <typename Callback>
    void on_shutdown(Callback cb) {
        on_shutdown_cb_ = cb;
    }
    /**
     * Begin waiting for requests.
     */
    void start();
    /**
     * Stop waiting for requests and close the session.
     */
    void stop();

   private:
    http::message_generator handle_request(
        http::request<http::string_body>&& req);
    void do_read();

    void do_stop(char const* reason);
    void on_read(beast::error_code ec, std::size_t bytes_transferred);

    void send_response(http::message_generator&& msg);

    void on_write(bool keep_alive,
                  beast::error_code ec,
                  std::size_t bytes_transferred);
};
