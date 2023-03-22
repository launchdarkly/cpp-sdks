#pragma once

#include "definitions.hpp"

#include <nlohmann/json.hpp>
#include <string>
#include <launchdarkly/sse/sse.hpp>
#include <boost/url/parse.hpp>
#include <thread>
#include <iostream>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// Report a failure
void fail(beast::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

class stream_entity  :public std::enable_shared_from_this<stream_entity> {
    std::string callback_url_;
    std::string callback_port_;
    std::string callback_host_;
    size_t callback_counter_;
    std::shared_ptr<launchdarkly::sse::client> client_;
    net::any_io_executor executor_;
    tcp::resolver resolver_;
    beast::tcp_stream stream_;
    http::request<http::string_body> req_;
public:
    stream_entity(net::any_io_executor executor, config_params params):
        callback_url_{params.callbackUrl},
       callback_host_{},
       callback_port_{},
        callback_counter_{0},
        client_{},
        executor_{executor},
       resolver_{executor},
       stream_{executor},
       req_{}
    {
        auto builder = launchdarkly::sse::builder{executor, params.streamUrl};
        if (params.headers) {
            for (auto h: *params.headers) {
                builder.header(h.first, h.second);
            }
        }

        boost::system::result<boost::urls::url_view> uri_components =
            boost::urls::parse_uri(params.callbackUrl);


        req_.set(http::field::host, uri_components->host());
        req_.method(http::verb::get);

        callback_host_ = uri_components->host();
        callback_port_ = uri_components->port();


        client_ = builder.build();
        if (client_) {
            client_->on_event([this](launchdarkly::sse::event_data ev) {
                req_.target(callback_url_+"/" + std::to_string(++callback_counter_));
                if (ev.get_type() == "comment") {
                    nlohmann::json json = comment_message{"comment", ev.get_data()};
                    req_.body() = json.dump();
                } else {
                    nlohmann::json json = event_message{"event", event{ev.get_type(), ev.get_data()}};
                    req_.body() = json.dump();
                }
                req_.prepare_payload();
                resolver_.async_resolve(
                    callback_host_, callback_port_,
                    beast::bind_front_handler(&stream_entity::on_resolve, shared_from_this()));
               });
//            client_->on_event([this, url = params.callbackUrl](launchdarkly::sse::event_data ev) {
//                tcp::resolver resolver{executor_};
//                beast::tcp_stream stream{executor_};
//
//                // Look up the domain name
//                auto const results = resolver.resolve(url, "80");
//
//                // Make the connection on the IP address we get from a lookup
//                stream.connect(results);
//
//                // Set up an HTTP GET request message
//                http::request<http::string_body> req{
//                        http::verb::get, url + "/" + std::to_string(++callback_counter_), 11};
//
//                if (ev.get_type() == "comment") {
//                    // comment_message thing2
//                    req.body() = nlohmann::json{
//                            comment_message{"comment", ev.get_data()}
//                    }.dump();
//                } else {
//                    req.body() = nlohmann::json{
//                            event_message{"event", event{ev.get_type(), ev.get_data()}}
//                    }.dump();
//                }
//                // Send the HTTP request to the remote host
//                http::write(stream, req);
//            });

            client_->read();
        }
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec)
            return fail(ec, "resolve");
        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(stream_).async_connect(
            results,
            beast::bind_front_handler(&stream_entity::on_connect, shared_from_this()));
    }

    void on_connect(beast::error_code ec,
                    tcp::resolver::results_type::endpoint_type) {
        if (ec)
            return fail(ec, "connect");

        http::async_write(
            stream_, req_,
            beast::bind_front_handler(&stream_entity::on_write, shared_from_this()));
    }

    void on_write(beast::error_code ec, std::size_t) {
        if (ec)
            return fail(ec, "write");

        stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes so don't bother reporting it.
        if(ec && ec != beast::errc::not_connected)
            return fail(ec, "shutdown");
    }

};
