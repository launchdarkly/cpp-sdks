#pragma once

#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <vector>
#include "entity_manager.hpp"

namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

class Session : public std::enable_shared_from_this<Session> {
    // The socket for the currently connected client.
    beast::tcp_stream stream_;

    // The buffer for performing reads.
    beast::flat_buffer buffer_{8192};

    // The request message.
    http::request<http::string_body> request_;

    EntityManager& manager_;

    std::vector<std::string> capabilities_;

    std::function<void()> on_shutdown_cb_;

    bool shutdown_requested_;

   public:
    explicit Session(tcp::socket&& socket,
                     EntityManager& manager,
                     std::vector<std::string> caps);

    ~Session();
    template <typename Callback>
    void on_shutdown(Callback cb) {
        on_shutdown_cb_ = cb;
    }

    void start();

    void stop();

   private:
    http::message_generator handle_request(
        http::request<http::string_body>&& req);
    void do_read();

    void do_stop();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void do_close();

    void send_response(http::message_generator&& msg);

    void on_write(bool keep_alive,
                  beast::error_code ec,
                  std::size_t bytes_transferred);
};

using SessionPtr = std::shared_ptr<Session>;
