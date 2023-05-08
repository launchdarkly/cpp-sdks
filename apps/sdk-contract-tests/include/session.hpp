#pragma once

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
    using Request = http::request<http::string_body>;
    using Response = http::response<http::string_body>;

    struct Frame {
        Request request_;
        Response resp_;
    };

    /**
     * Constructs a session, which provides a REST API.
     * @param session The HTTP session.
     * @param manager Manager through which entities can be created/destroyed.
     * @param caps Test service capabilities to advertise.
     * @param logger Logger.
     */
    Session(foxy::server_session& session,
            EntityManager& manager,
            std::vector<std::string>& caps,
            launchdarkly::Logger& logger);

    template <class Self>
    auto operator()(Self& self,
                    boost::system::error_code ec = {},
                    std::size_t const bytes_transferred = 0) -> void {
        using launchdarkly::LogLevel;
        auto& f = *frame_;

        reenter(*this) {
            while (true) {
                f.resp_ = {};
                f.request_ = {};

                yield session_.async_read(f.request_, std::move(self));
                if (ec) {
                    LD_LOG(logger_, LogLevel::kWarn)
                        << "session: read: " << ec.what();
                    break;
                }

                if (auto response = generate_response(f.request_)) {
                    f.resp_ = *response;
                } else {
                    LD_LOG(logger_, LogLevel::kWarn)
                        << "session: shutdown requested by client";
                    std::exit(0);
                }

                yield session_.async_write(f.resp_, std::move(self));

                if (ec) {
                    LD_LOG(logger_, LogLevel::kWarn)
                        << "session: write: " << ec.what();
                    break;
                }

                if (!f.request_.keep_alive()) {
                    break;
                }
            }

            return self.complete({}, 0);
        }
    }

    std::optional<Response> generate_response(Request& req);

   private:
    foxy::server_session& session_;
    std::unique_ptr<Frame> frame_;
    std::vector<std::string>& caps_;
    launchdarkly::Logger& logger_;
};

#include <boost/asio/unyield.hpp>
