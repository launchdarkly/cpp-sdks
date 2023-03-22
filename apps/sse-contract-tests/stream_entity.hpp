#pragma once

#include "definitions.hpp"

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

// Report a failure
void fail(beast::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

class stream_entity : public std::enable_shared_from_this<stream_entity> {
    using request_type = http::request<http::string_body>;

    std::string callback_url_;
    std::string callback_port_;
    std::string callback_host_;
    size_t callback_counter_;
    std::shared_ptr<launchdarkly::sse::client> client_;
    net::any_io_executor executor_;
    tcp::resolver resolver_;
    beast::tcp_stream stream_;
    std::deque<request_type> requests_;

    request_type build_request(std::size_t counter,
                               launchdarkly::sse::event_data ev) {
        request_type req;
        req.set(http::field::host, callback_host_);
        req.method(http::verb::get);
        req.target(callback_url_ + "/" + std::to_string(counter));
        if (ev.get_type() == "comment") {
            nlohmann::json json = comment_message{"comment", ev.get_data()};
            req.body() = json.dump();
        } else {
            nlohmann::json json = event_message{
                "event",
                event{ev.get_type(), ev.get_data(), ev.get_id().value_or("")}};
            req.body() = json.dump();
        }
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

        request_type& req = requests_.front();
        std::cout << "writing event: " << req.body() << '\n';
        http::async_write(stream_, req,
                          beast::bind_front_handler(&stream_entity::on_write,
                                                    shared_from_this()));
    }

    void on_write(beast::error_code ec, std::size_t) {
        if (ec)
            return fail(ec, "write");

        requests_.pop_front();
        stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes so don't bother reporting it.
        if (ec && ec != beast::errc::not_connected)
            return fail(ec, "shutdown");
    }

   public:
    stream_entity(net::any_io_executor executor, config_params params)
        : callback_url_{params.callbackUrl},
          callback_host_{},
          callback_port_{},
          callback_counter_{0},
          client_{},
          executor_{executor},
          resolver_{executor},
          stream_{executor},
          requests_{} {
        auto builder = launchdarkly::sse::builder{executor, params.streamUrl};
        if (params.headers) {
            for (auto h : *params.headers) {
                builder.header(h.first, h.second);
            }
        }

        boost::system::result<boost::urls::url_view> uri_components =
            boost::urls::parse_uri(params.callbackUrl);

        callback_host_ = uri_components->host();
        callback_port_ = uri_components->port();

        client_ = builder.build();
        if (client_) {
            client_->on_event([this](launchdarkly::sse::event_data ev) {
                auto req = build_request(callback_counter_++, std::move(ev));
                requests_.push_back(req);
                resolver_.async_resolve(
                    callback_host_, callback_port_,
                    beast::bind_front_handler(&stream_entity::on_resolve,
                                              shared_from_this()));
            });
            client_->read();
        }
    }
};
