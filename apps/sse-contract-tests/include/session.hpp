#pragma once

#include "entity_manager.hpp"
#include "logger.hpp"

#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <foxy/server_session.hpp>
#include <vector>

#include <boost/asio/yield.hpp>

namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

class Session : boost::asio::coroutine {
   public:
    /**
     * Constructs a session, which provides a REST API.
     * @param socket Connected socket.
     * @param manager Manager through which entities can be created/destroyed.
     * @param caps Test service capabilities to advertise.
     * @param logger Logger.
     */
    explicit Session(foxy::server_session& session,
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

    template <class Self>
    auto operator()(Self& self,
                    boost::system::error_code ec = {},
                    std::size_t const bytes_transferred = 0) -> void {
        reenter(*this) {
            while (true) {
                resp_ = {};
                request_ = {};

                yield session_.async_read(request_, std::move(self));
                if (ec) {
                    break;
                }

                resp_ = generate_response(request_);

                yield session_.async_write(resp_, std::move(self));

                if (ec) {
                    break;
                }

                if (!request_.keep_alive()) {
                    break;
                }
            }

            return self.complete({}, 0);
        }
    }

   private:
    foxy::server_session& session_;

    http::request<http::string_body> request_;
    http::response<http::string_body> resp_;
    EntityManager& manager_;
    std::vector<std::string> capabilities_;
    std::function<void()> on_shutdown_cb_;
    bool shutdown_requested_;
    launchdarkly::Logger& logger_;

    http::response<http::string_body> generate_response(
        http::request<http::string_body>& req);

    void do_stop(char const* reason);
};

#include <boost/asio/unyield.hpp>
