#pragma once

#include "definitions.hpp"

#include <nlohmann/json.hpp>
#include <string>
#include <launchdarkly/sse/sse.hpp>
#include <thread>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


class stream_entity {
    std::string callback_url_;
    size_t callback_counter_;
    std::shared_ptr<launchdarkly::sse::client> client_;
    net::any_io_executor executor_;
public:
    stream_entity(net::any_io_executor executor, config_params params):
        callback_url_{params.callbackUrl},
        callback_counter_{0},
        client_{},
        executor_{executor}
    {
        auto builder = launchdarkly::sse::builder{executor, params.streamUrl};
        if (params.headers) {
            for (auto h: *params.headers) {
                builder.header(h.first, h.second);
            }
        }


        client_ = builder.build();
        if (client_) {
            client_->on_event([this, url = params.callbackUrl](launchdarkly::sse::event_data ev) {
                tcp::resolver resolver{executor_};
                beast::tcp_stream stream{executor_};

                // Look up the domain name
                auto const results = resolver.resolve(url, "80");

                // Make the connection on the IP address we get from a lookup
                stream.connect(results);

                // Set up an HTTP GET request message
                http::request<http::string_body> req{
                        http::verb::get, url + "/" + std::to_string(++callback_counter_), 11};

                if (ev.get_type() == "comment") {
                    // comment_message thing2
                    req.body() = nlohmann::json{
                            comment_message{"comment", ev.get_data()}
                    }.dump();
                } else {
                    req.body() = nlohmann::json{
                            event_message{"event", event{ev.get_type(), ev.get_data()}}
                    }.dump();
                }
                // Send the HTTP request to the remote host
                http::write(stream, req);
            });

            client_->read();
        }
    }
};
