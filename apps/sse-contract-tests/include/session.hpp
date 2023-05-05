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
    struct Frame {
        http::request<http::string_body> request_;
        http::response<http::string_body> resp_;
    };
    /**
     * Constructs a session, which provides a REST API.
     * @param socket Connected socket.
     * @param manager Manager through which entities can be created/destroyed.
     * @param caps Test service capabilities to advertise.
     * @param logger Logger.
     */
    Session(foxy::server_session& session,
            EntityManager& manager,
            std::vector<std::string> caps,
            launchdarkly::Logger& logger);

    /**
     * Set a callback to be invoked when a REST client requests shutdown.
     */
    template <typename Callback>
    void on_shutdown(Callback cb) {
        // on_shutdown_cb_ = cb;
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
        auto& f = *frame_;
        reenter(*this) {
            while (true) {
                f.resp_ = {};
                f.request_ = {};

                yield session_.async_read(f.request_, std::move(self));
                if (ec) {
                    break;
                }

                f.resp_ = generate_response(f.request_);

                if (shutdown_requested_) {
                    break;
                }

                yield session_.async_write(f.resp_, std::move(self));

                if (ec) {
                    break;
                }

                if (!f.request_.keep_alive()) {
                    break;
                }
            }

            return self.complete({}, 0);
        }
    }

    http::response<http::string_body> generate_response(
        http::request<http::string_body>& req);

   private:
    foxy::server_session& session_;
    std::unique_ptr<Frame> frame_;
    EntityManager& manager_;
    std::vector<std::string> caps_;
    std::function<void()> on_shutdown_cb_;
    bool shutdown_requested_;
    launchdarkly::Logger& logger_;

    void do_stop(char const* reason);
};

#include <boost/asio/unyield.hpp>
